#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "fft_acc"
#define FFT_ACC_BASE_ADDR 0x40000000
#define FFT_ACC_REG_SIZE  0x100

// Register offsets
#define DEVID   0x000
#define CTRL    0x001
#define CFG0    0x002
#define DATAIN  0x003
#define DATAOUT 0x004
#define STATUS  0x005

// Control Register Bits
#define EN_FIELD  (1 << 0)
#define IEN_FIELD (1 << 1)
#define SAM_FIELD (1 << 2)
#define PRC_FIELD (1 << 3)

// Status Register Bits
#define EMPTY_FIELD (1 << 4)
#define FULL_FIELD  (1 << 3)
#define SCPLT_FIELD (1 << 1)
#define PCPLT_FIELD (1 << 2)
#define READY_FIELD (1 << 0)

static void __iomem *fft_acc_base;
static dev_t fft_dev;
static struct cdev fft_cdev;
static struct class *fft_class;

// IOCTL Commands
#define FFT_ACC_RESET     _IO('F', 0)
#define FFT_ACC_START     _IO('F', 1)
#define FFT_ACC_SET_CFG   _IOW('F', 2, uint32_t)
#define FFT_ACC_LOAD_DATA _IOW('F', 3, uint32_t)
#define FFT_ACC_GET_RESULT _IOR('F', 4, uint32_t)

static int fft_acc_open(struct inode *inode, struct file *file) {
    return 0;
}

static int fft_acc_release(struct inode *inode, struct file *file) {
    return 0;
}

static long fft_acc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    uint32_t val;
    switch (cmd) {
        case FFT_ACC_RESET:
            // Deassert EN to reset
            iowrite32(0, fft_acc_base + CTRL);
            // Wait for READY flag
            while (!(ioread32(fft_acc_base + STATUS) & READY_FIELD));
            break;
        case FFT_ACC_START:
            val = ioread32(fft_acc_base + CTRL) | PRC_FIELD;
            iowrite32(val, fft_acc_base + CTRL);
            break;
        case FFT_ACC_SET_CFG:
            if (copy_from_user(&val, (uint32_t __user *)arg, sizeof(val)))
                return -EFAULT;
            iowrite32(val, fft_acc_base + CFG0);
            break;
        case FFT_ACC_LOAD_DATA:
            if (copy_from_user(&val, (uint32_t __user *)arg, sizeof(val)))
                return -EFAULT;
            
            // Enable device before loading samples
            iowrite32(EN_FIELD, fft_acc_base + CTRL);
            
            // Load sample
            iowrite32(val, fft_acc_base + DATAIN);
            val = ioread32(fft_acc_base + CTRL) | SAM_FIELD;
            iowrite32(val, fft_acc_base + CTRL);
            
            // Wait for SCPLT before loading next sample
            while (!(ioread32(fft_acc_base + STATUS) & SCPLT_FIELD));
            
            // Check if FULL flag is set
            if (ioread32(fft_acc_base + STATUS) & FULL_FIELD)
                pr_info("FFT Accelerator: Sample buffer FULL\n");
            
            break;
        case FFT_ACC_GET_RESULT:
            {
                uint32_t result;
                uint32_t i = 0;
                uint32_t status;
                
                // Deassert EN before reading results
                val = ioread32(fft_acc_base + CTRL) & ~EN_FIELD;
                iowrite32(val, fft_acc_base + CTRL);
                
                while (!(ioread32(fft_acc_base + STATUS) & EMPTY_FIELD)) {
                    // Set SAM to request next result
                    val = ioread32(fft_acc_base + CTRL) | SAM_FIELD;
                    iowrite32(val, fft_acc_base + CTRL);
                    
                    // Wait for PCPLT flag
                    do {
                        status = ioread32(fft_acc_base + STATUS);
                    } while (!(status & PCPLT_FIELD));
                    
                    // Read result
                    result = ioread32(fft_acc_base + DATAOUT);
                    if (copy_to_user((uint32_t __user *)(arg + i * sizeof(uint32_t)), &result, sizeof(uint32_t)))
                        return -EFAULT;
                    i++;
                }
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations fft_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = fft_acc_ioctl,
    .open = fft_acc_open,
    .release = fft_acc_release,
};

static int __init fft_acc_init(void) {
    int ret;
    fft_acc_base = ioremap(FFT_ACC_BASE_ADDR, FFT_ACC_REG_SIZE);
    if (!fft_acc_base) {
        pr_err("Failed to map FFT accelerator registers\n");
        return -ENOMEM;
    }
    ret = alloc_chrdev_region(&fft_dev, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;
    cdev_init(&fft_cdev, &fft_fops);
    cdev_add(&fft_cdev, fft_dev, 1);
    fft_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(fft_class, NULL, fft_dev, NULL, DEVICE_NAME);
    pr_info("FFT Accelerator driver initialized\n");
    return 0;
}

static void __exit fft_acc_exit(void) {
    device_destroy(fft_class, fft_dev);
    class_destroy(fft_class);
    cdev_del(&fft_cdev);
    unregister_chrdev_region(fft_dev, 1);
    iounmap(fft_acc_base);
    pr_info("FFT Accelerator driver removed\n");
}

module_init(fft_acc_init);
module_exit(fft_acc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("FFT Accelerator Kernel Driver");