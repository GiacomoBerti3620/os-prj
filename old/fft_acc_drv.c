/*
 * FFT Accelerator Device Driver
 *
 * Implements reading/writing samples, processing FFT, and handling interrupts.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/wait.h>
#include <linux/mutex.h>

#define REG_ID 0x000
#define REG_CTRL 0x001
#define REG_CFG0 0x002
#define REG_DATAIN 0x003
#define REG_DATAOUT 0x004
#define REG_STATUS 0x005

#define CTRL_EN BIT(0)
#define CTRL_I_EN BIT(1)
#define CTRL_SAM BIT(2)
#define CTRL_PRC BIT(3)

#define STATUS_READY BIT(0)
#define STATUS_SCPLT BIT(1)
#define STATUS_PCPLT BIT(2)
#define STATUS_FULL BIT(3)
#define STATUS_EMPTY BIT(4)

struct fft_accel_dev
{
    struct device *dev;
    void __iomem *base;
    int irq;
    wait_queue_head_t wq;
    struct mutex lock;
    int status;
};

static irqreturn_t fft_irq_handler(int irq, void *data)
{
    struct fft_accel_dev *fft = data;
    u32 status = readl_relaxed(fft->base + REG_STATUS);

    fft->status = status;
    wake_up_interruptible(&fft->wq);
    return IRQ_HANDLED;
}

static ssize_t fft_start_process(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
    struct fft_accel_dev *fft = dev_get_drvdata(dev);
    mutex_lock(&fft->lock);
    writel_relaxed(CTRL_PRC, fft->base + REG_CTRL);
    mutex_unlock(&fft->lock);
    return len;
}

static ssize_t fft_read_result(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fft_accel_dev *fft = dev_get_drvdata(dev);
    wait_event_interruptible(fft->wq, fft->status & STATUS_PCPLT);
    return scnprintf(buf, PAGE_SIZE, "Result: 0x%x\n", readl_relaxed(fft->base + REG_DATAOUT));
}

static DEVICE_ATTR(start, S_IWUSR, NULL, fft_start_process);
static DEVICE_ATTR(result, S_IRUGO, fft_read_result, NULL);

static struct attribute *fft_attributes[] = {
    &dev_attr_start.attr,
    &dev_attr_result.attr,
    NULL,
};

static const struct attribute_group fft_attr_group = {
    .attrs = fft_attributes,
};

static int fft_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct fft_accel_dev *fft;
    struct resource *res;
    int ret;

    fft = devm_kzalloc(dev, sizeof(*fft), GFP_KERNEL);
    if (!fft)
        return -ENOMEM;

    fft->dev = dev;
    mutex_init(&fft->lock);
    init_waitqueue_head(&fft->wq);
    platform_set_drvdata(pdev, fft);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    fft->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(fft->base))
        return PTR_ERR(fft->base);

    fft->irq = platform_get_irq(pdev, 0);
    if (fft->irq > 0)
    {
        ret = devm_request_irq(dev, fft->irq, fft_irq_handler, 0, pdev->name, fft);
        if (ret)
            return ret;
    }

    return sysfs_create_group(&dev->kobj, &fft_attr_group);
}

static int fft_remove(struct platform_device *pdev)
{
    struct fft_accel_dev *fft = platform_get_drvdata(pdev);
    sysfs_remove_group(&fft->dev->kobj, &fft_attr_group);
    return 0;
}

static const struct of_device_id fft_of_match[] = {
    {
        .compatible = "virt-fft-acc",
    },
    {}};
MODULE_DEVICE_TABLE(of, fft_of_match);

static struct platform_driver fft_driver = {
    .probe = fft_probe,
    .remove = fft_remove,
    .driver = {
        .name = "fft_acc_drv",
        .of_match_table = fft_of_match,
    },
};
module_platform_driver(fft_driver);

MODULE_DESCRIPTION("FFT Accelerator Driver with Interrupts");
MODULE_AUTHOR("Samz and Jack");
MODULE_LICENSE("GPL");
