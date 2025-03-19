/*
 * Virtual Foo Device Driver
 *
 * Copyright 2017 Milo Kim <woogyom.kim@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

/* Register map */
#define DEVID 0x000
#define ID 0xfacecafe

#define CTRL 0x001
#define EN_FIELD ((uint32_t)1U << 0)
#define IEN_FIELD ((uint32_t)1U << 1)
#define SAM_FIELD ((uint32_t)1U << 2)
#define PRC_FIELD ((uint32_t)1U << 3)

#define CFG0 0x002
#define SMODE_FIELD ((uint32_t)1U << 0)
#define NSAMPLES_MASK 0x000e
#define NSAMPLES_SH 0x1

#define DATAIN 0x003
#define DATAIN_H_MASK 0xff00
#define DATAIN_H_SH 0x8
#define DATAIN_L_MASK 0x00ff
#define DATAIN_L_SH 0x0

#define DATAOUT 0x004

#define STATUS 0x005
#define READY_FIELD ((uint32_t)1U << 0)
#define SCPLT_FIELD ((uint32_t)1U << 1)
#define PCPLT_FIELD ((uint32_t)1U << 2)
#define FULL_FIELD ((uint32_t)1U << 3)
#define EMPTY_FIELD ((uint32_t)1U << 4)

// Number of samples processed max (actual number set in NSAMPLES)
#define SAMPLES_COUNT 2048

typedef enum {
    CTRL_EN,
    CTRL_I_EN,
    CTRL_SAM,
    CTRL_PRC,
} e_controlField;

typedef enum {
    CFG_SMODE,
    CFG_NSAMPLES,
} e_configField;

typedef enum {
    DATAIN_DATAIN_LOW,
    DATAIN_DATAIN_HIGH,
} e_datainField;

struct virt_fft_acc
{
    struct device *dev;
    void __iomem *base;
};

// >> Read Id register

static ssize_t vf_show_id(struct device *dev, char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + DEVID);

    return scnprintf(buf, PAGE_SIZE, "Chip ID: 0x%.x\\n", val);
}

// >> Read Status register

static u32 vf_show_status(struct device *dev)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + STATUS);

    return val;
}
// >> Write on Control register

static int vf_store_ctrl_en(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CTRL);

    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    val = (prevval & ~EN_FIELD) | (val * EN_FIELD);
    writel_relaxed(val, vf->base + CTRL);

    return 0;
}

static int vf_store_ctrl_ien(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CTRL);

    if (kstrtoul(buf, 0, &val))
        return -1;

    val = (prevval & ~IEN_FIELD) | (val * IEN_FIELD);
    writel_relaxed(val, vf->base + CTRL);

    return 0;
}

static int vf_store_ctrl_sam(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CTRL);

    if (kstrtoul(buf, 0, &val))
        return -1;

    val = (prevval & ~SAM_FIELD) | (val * SAM_FIELD);
    writel_relaxed(val, vf->base + CTRL);

    return 0;
}

static int vf_store_ctrl_prc(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CTRL);

    if (kstrtoul(buf, 0, &val))
        return -1;

    val = (prevval & ~PRC_FIELD) | (val * PRC_FIELD);
    writel_relaxed(val, vf->base + CTRL);

    return 0;
}

// >> Store control wrapper
static int vf_store_ctrl(struct device *dev, const char *buf,
                         e_controlField ctrl_field)
{
    switch (ctrl_field)
    {
    case CTRL_EN:
        len = vf_store_ctrl_en(dev, buf);
        break;
    case CTRL_I_EN:
        len = vf_store_ctrl_ien(dev, buf);
        break;
    case CTRL_SAM:
        len = vf_store_ctrl_sam(dev, buf);
        break;
    case CTRL_PRC:
        len = vf_store_ctrl_prc(dev, buf);
        break;
    default:
        return -1
    }

    return 0;
}

// >> Write on Configuration register

static int vf_store_cfg_smode(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CFG0);

    if (kstrtoul(buf, 0, &val))
        return -1;

    val = (prevval & ~SMODE_FIELD) | (val * SMODE_FIELD);
    writel_relaxed(val, vf->base + CFG0);

    return 0;
}

static int vf_store_cfg_nsamples(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;
    u32 prevval = readl_relaxed(vf->base + CFG0);

    if (kstrtoul(buf, 0, &val))
        return -1;

    val = (prevval & ~NSAMPLES_MASK) | (val * NSAMPLES_SH);
    writel_relaxed(val, vf->base + CFG0);

    return 0;
}

// >> Store config wrapper
static int vf_store_cfg(struct device *dev,
                        const char *buf, e_configField store_field)
{
    switch (store_field)
    {
    case CFG_SMODE:
        return vf_store_cfg_smode(dev, attr, buf, len);
    case CFG_NSAMPLES:
        return vf_store_cfg_nsamples(dev, attr, buf, len);
    default:
        return -1;
    }
}

// >> Write DATAIN register

static int vf_write_32data(struct device *dev, const char *buf)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val))
        return -1;

    writel_relaxed(val, vf->base + DATAIN);

    return 0;
}

static int vf_write_2_16data(struct device *dev,
                             char *buf0, char *buf1)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    unsigned long val, val0, val1;

    if (kstrtoul(buf0, 0, &val0))
        return -EINVAL;
    if (kstrtoul(buf1, 0, &val1))
        return -1;
    val = val0 | (val1 << DATAIN_H_SH);

    writel_relaxed(val, vf->base + DATAIN);

    return 0;
}

// >> Write datain wrapper
static int vf_write_data(struct device *dev, const char *buf0,
                         const char *buf1, e_datainField data_field)
{
    switch (data_field)
    {
    case DATAIN_DATAIN_LOW:
        return vf_write_32data(dev, attr, buf0);
    case DATAIN_DATAIN_HIGH:
        return vf_write_2_16data(dev, attr, buf0, buf1);
    default:
        return -1;
    }

    return 0;
}

// >> Read DATAOUT register

static u32 vf_read_dataout(struct device *dev)
{
    struct virt_fft_acc *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + DATAOUT);

    return val;
}

static DEVICE_ATTR(id, S_IRUGO, vf_show_id, NULL);
static DEVICE_ATTR(ctrl, S_IRUGO | S_IWUSR, NULL, vf_store_ctrl);
static DEVICE_ATTR(cfg, S_IRUGO | S_IWUSR, NULL, vf_store_cfg);
static DEVICE_ATTR(datain, S_IWUSR, NULL, vf_write_data);
static DEVICE_ATTR(dataout, S_IRUGO, vf_read_dataout, NULL);
static DEVICE_ATTR(status, S_IRUGO, vf_show_status, NULL);

static struct attribute *vf_attributes[] = {
    &dev_attr_id.attr,
    &dev_attr_ctrl.attr,
    &dev_attr_cfg.attr,
    &dev_attr_datain.attr,
    &dev_attr_dataout.attr,
    &dev_attr_status.attr,
    NULL,
};

static const struct attribute_group vf_attr_group = {
    .attrs = vf_attributes,
};

static void vf_init(struct virt_fft_acc *vf)
{
    u32 ctrl_reg, ctrl_reg_nen;
    ctrl_reg = readl_relaxed(vf->base + CTRL);
    ctrl_reg_nen = ctrl_reg & ~EN_FIELD;

    writel_relaxed(ctrl_reg_nen, vf->base + CTRL);
}

void fft_operation(int n_samples, int s_mode,
                   unsigned long *data_in, unsigned long *data_out)
{
    int fd, i;
    char n_samples_char, s_mode_char;
    char buf0[];
    char buf1[];
    e_configField my_configField;
    e_controlField my_controlField;
    e_datainField my_datainField;

    fd = open("/dev/virt_fft_acc", O_RDWR);

    vf_store_ctrl(fd, "0", CTRL_EN);

    switch (n_samples)
    {
    case 16:
        n_samples_char = "0";
        break;
    case 32:
        n_samples_char = "1";
        break;
    case 64:
        n_samples_char = "2";
        break;
    case 128:
        n_samples_char = "3";
        break;
    case 256:
        n_samples_char = "4";
        break;
    case 512:
        n_samples_char = "5";
        break;
    case 1024:
        n_samples_char = "6";
        break;
    case 2048:
        n_samples_char = "7";
        break;
    default:
        break;
    }

    switch (s_mode)
    {
    case 0:
        s_mode_char = "0";
        break;
    case 1:
        s_mode_char = "1";
        break;
    default:
        break;
    }

    my_configField = CFG_NSAMPLES;
    vf_store_cfg(fd, n_samples_char, my_configField);
    my_configField = CFG_SMODE;
    vf_store_cfg(fd, s_mode_char, my_configField);
    my_controlField = CTRL_EN;
    vf_store_ctrl(fd, "1", my_configField);

    i = 0;
    while (i < n_samples)
    {
        if (s_mode_char == "0")
            my_datainField = DATAIN_DATAIN_LOW;
            vf_write_data(fd, datain, (char *)data_in[i], (char *)data_in[i],  my_datainField);
            i++;

        if (s_mode_char == "1")
            my_datainField = DATAIN_DATAIN_HIGH;
            vf_write_data(fd, datain, (char *)(data_in[i] & DATAIN_L_MASK), (char *)((data_in[i+1] | DATAIN_H_MASK) >> DATAIN_H_SH),  my_datainField);
            i = i +2;
    
        vf_store_ctrl(fd, "1", CTRL_SAM);
        while ( vf_show_status(fd) & SCPLT_FIELD == 0 );
    }

    my_controlField = CTRL_PRC;
    vf_store_ctrl(fd, "1", my_controlField);
    my_controlField = CTRL_EN;
    vf_store_ctrl(fd, "0", my_controlField);

    i = 0;
    while (vf_show_status(fd) & EMPTY_FIELD == 0)
    {
        vf_store_ctrl(fd, "1", CTRL_SAM);
        while ( vf_show_status(fd) & PCPLT_FIELD == 0 );
        data_out[i] = vf_read_dataout(fd);
    }

    my_controlField = CTRL_EN;
    vf_store_ctrl(fd, "1", my_controlField);

}

static irqreturn_t vf_irq_handler(int irq, void *data)
{
    struct virt_fft_acc *vf = (struct virt_fft_acc *)data;
    u32 status;

    status = readl_relaxed(vf->base + STATUS);

    if (status & READY_FIELD)
        dev_info(vf->dev, "Ready pin high\\n");

    if (status & SCPLT_FIELD)
        dev_info(vf->dev, "SCPLT pin high\\n");

    if (status & PCPLT_FIELD)
        dev_info(vf->dev, "PCPLT pin high\\n");

    if (status & FULL_FIELD)
        dev_info(vf->dev, "FULL pin high\\n");

    if (status & EMPTY_FIELD)
        dev_info(vf->dev, "EMPTY pin high\\n");

    return IRQ_HANDLED;
}

static int vf_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    struct virt_fft_acc *vf;
    int ret;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENOMEM;

    vf = devm_kzalloc(dev, sizeof(*vf), GFP_KERNEL);
    if (!vf)
        return -ENOMEM;

    vf->dev = dev;
    vf->base = devm_ioremap(dev, res->start, resource_size(res));
    if (!vf->base)
        return -EINVAL;

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res)
    {
        ret = devm_request_irq(dev, res->start, vf_irq_handler,
                               IRQF_TRIGGER_HIGH, "vf_irq", vf);
        if (ret)
            return ret;
    }

    platform_set_drvdata(pdev, vf);

    vf_init(vf);

    return sysfs_create_group(&dev->kobj, &vf_attr_group);
}

static int vf_remove(struct platform_device *pdev)
{
    struct virt_fft_acc *vf = platform_get_drvdata(pdev);

    sysfs_remove_group(&vf->dev->kobj, &vf_attr_group);
    return 0;
}

static const struct of_device_id vf_of_match[] = {
    {
        .compatible = "virt-fft-acc",
    },
    {}};
MODULE_DEVICE_TABLE(of, vf_of_match);

static struct platform_driver vf_driver = {
    .probe = vf_probe,
    .remove = vf_remove,
    .driver = {
        .name = "virt_fft_acc",
        .of_match_table = vf_of_match,
    },
};
module_platform_driver(vf_driver);

MODULE_DESCRIPTION("Virtual FFT Accellerator Driver");