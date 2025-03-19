#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>

#define DEVICE_NAME "virt_fft_acc"
#define CLASS_NAME "virt_fft"

static int majorNumber;
static struct class* virt_fft_class = NULL;
static struct device* virt_fft_device = NULL;
static struct cdev virt_fft_cdev;

// Define the base address of the device
#define VIRT_FFT_BASE_ADDR 0x10000000

// Define the register offsets
#define DEVID 0x000
#define CTRL 0x001
#define CFG0 0x002
#define DATAIN 0x003
#define DATAOUT 0x004
#define STATUS 0x005

// Function prototypes
static int virt_fft_open(struct inode *, struct file *);
static int virt_fft_release(struct inode *, struct file *);
static ssize_t virt_fft_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t virt_fft_write(struct file *, const char __user *, size_t, loff_t *);
static long virt_fft_ioctl(struct file *, unsigned int, unsigned long);

// File operations structure
static struct file_operations fops = {
    .open = virt_fft_open,
    .release = virt_fft_release,
    .read = virt_fft_read,
    .write = virt_fft_write,
    .unlocked_ioctl = virt_fft_ioctl,
};

static int virt_fft_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "virt_fft: Device opened\n");
    return 0;
}

static int virt_fft_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "virt_fft: Device closed\n");
    return 0;
}

static ssize_t virt_fft_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    uint32_t data;
    // Read from DATAOUT register
    data = ioread32(VIRT_FFT_BASE_ADDR + DATAOUT);
    if (copy_to_user(buffer, &data, sizeof(data))) {
        return -EFAULT;
    }
    return sizeof(data);
}

static ssize_t virt_fft_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    uint32_t data;
    if (copy_from_user(&data, buffer, sizeof(data))) {
        return -EFAULT;
    }
    // Write to DATAIN register
    iowrite32(data, VIRT_FFT_BASE_ADDR + DATAIN);
    return sizeof(data);
}

static void virt_fft_reset(void) {
    uint32_t ctrl_value;

    // Read the current value of the CTRL register
    ctrl_value = ioread32(VIRT_FFT_BASE_ADDR + CTRL);

    // Deassert the EN signal (set EN_FIELD to 0)
    ctrl_value &= ~EN_FIELD;

    // Write the updated value back to the CTRL register
    iowrite32(ctrl_value, VIRT_FFT_BASE_ADDR + CTRL);

    // Check if the READY field in the STATUS register is asserted
    uint32_t status_value = ioread32(VIRT_FFT_BASE_ADDR + STATUS);
    if (status_value & READY_FIELD) {
        printk(KERN_INFO "virt_fft: Accelerator reset successfully, READY field asserted\n");
    } else {
        printk(KERN_WARNING "virt_fft: Accelerator reset failed, READY field not asserted\n");
    }
}

static int virt_fft_configure(uint32_t cfg_value) {
    // Write the configuration to the CFG0 register
    iowrite32(cfg_value, VIRT_FFT_BASE_ADDR + CFG0);
    printk(KERN_INFO "virt_fft: Configuration set to 0x%x\n", cfg_value);
    return 0;
}

static int virt_fft_load_sample(uint32_t sample_value) {
    uint32_t status_value;

    // Write the sample value to the DATAIN register
    iowrite32(sample_value, VIRT_FFT_BASE_ADDR + DATAIN);

    // Trigger the SAM field in the CTRL register
    uint32_t ctrl_value = ioread32(VIRT_FFT_BASE_ADDR + CTRL);
    ctrl_value |= SAM_FIELD;
    iowrite32(ctrl_value, VIRT_FFT_BASE_ADDR + CTRL);

    // Wait for the SCPLT field to be asserted
    do {
        status_value = ioread32(VIRT_FFT_BASE_ADDR + STATUS);
    } while (!(status_value & SCPLT_FIELD));

    printk(KERN_INFO "virt_fft: Sample loaded successfully, SCPLT field asserted\n");

    // Check if the FULL field is asserted
    if (status_value & FULL_FIELD) {
        printk(KERN_INFO "virt_fft: All samples loaded, FULL field asserted\n");
    }

    return 0;
}

static int virt_fft_process_samples(void) {
    uint32_t status_value;

    // Trigger the PRC field in the CTRL register
    uint32_t ctrl_value = ioread32(VIRT_FFT_BASE_ADDR + CTRL);
    ctrl_value |= PRC_FIELD;
    iowrite32(ctrl_value, VIRT_FFT_BASE_ADDR + CTRL);

    // Wait for the PCPLT field to be asserted
    do {
        status_value = ioread32(VIRT_FFT_BASE_ADDR + STATUS);
    } while (!(status_value & PCPLT_FIELD));

    printk(KERN_INFO "virt_fft: Samples processed successfully, PCPLT field asserted\n");

    return 0;
}

static int virt_fft_read_result(uint32_t *result) {
    uint32_t status_value;

    // Trigger the SAM field in the CTRL register
    uint32_t ctrl_value = ioread32(VIRT_FFT_BASE_ADDR + CTRL);
    ctrl_value |= SAM_FIELD;
    iowrite32(ctrl_value, VIRT_FFT_BASE_ADDR + CTRL);

    // Wait for the SCPLT field to be asserted
    do {
        status_value = ioread32(VIRT_FFT_BASE_ADDR + STATUS);
    } while (!(status_value & SCPLT_FIELD));

    // Read the result from the DATAOUT register
    *result = ioread32(VIRT_FFT_BASE_ADDR + DATAOUT);

    printk(KERN_INFO "virt_fft: Result read successfully, SCPLT field asserted\n");

    // Check if the EMPTY field is asserted
    if (status_value & EMPTY_FIELD) {
        printk(KERN_INFO "virt_fft: All results read, EMPTY field asserted\n");
    }

    return 0;
}

#define VIRT_FFT_IOCTL_RESET _IO('V', 1)       // Reset the accelerator
#define VIRT_FFT_IOCTL_CONFIGURE _IOW('V', 2, uint32_t) // Configure the device
#define VIRT_FFT_IOCTL_LOAD_SAMPLE _IOW('V', 3, uint32_t) // Load a sample
#define VIRT_FFT_IOCTL_PROCESS _IO('V', 4)     // Process the samples
#define VIRT_FFT_IOCTL_READ_RESULT _IOR('V', 5, uint32_t) // Read a result

static long virt_fft_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    uint32_t value;
    switch (cmd) {
        case VIRT_FFT_IOCTL_RESET: // Reset the accelerator
            virt_fft_reset();
            break;
        case VIRT_FFT_IOCTL_CONFIGURE: // Configure the device
            if (copy_from_user(&value, (uint32_t __user *)arg, sizeof(value))) {
                return -EFAULT;
            }
            virt_fft_configure(value);
            break;
        case VIRT_FFT_IOCTL_LOAD_SAMPLE: // Load a sample
            if (copy_from_user(&value, (uint32_t __user *)arg, sizeof(value))) {
                return -EFAULT;
            }
            virt_fft_load_sample(value);
            break;
        case VIRT_FFT_IOCTL_PROCESS: // Process the samples
            virt_fft_process_samples();
            break;
        case VIRT_FFT_IOCTL_READ_RESULT: // Read a result
            if (virt_fft_read_result(&value) < 0) {
                return -EFAULT;
            }
            if (copy_to_user((uint32_t __user *)arg, &value, sizeof(value))) {
                return -EFAULT;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int __init virt_fft_init(void) {
    printk(KERN_INFO "virt_fft: Initializing the virtual FFT accelerator driver\n");

    // Dynamically allocate a major number for the device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "virt_fft failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "virt_fft: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    virt_fft_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(virt_fft_class)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(virt_fft_class);
    }
    printk(KERN_INFO "virt_fft: device class registered correctly\n");

    // Register the device driver
    virt_fft_device = device_create(virt_fft_class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(virt_fft_device)) {
        class_destroy(virt_fft_class);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(virt_fft_device);
    }
    printk(KERN_INFO "virt_fft: device class created correctly\n");

    return 0;
}

static void __exit virt_fft_exit(void) {
    device_destroy(virt_fft_class, MKDEV(majorNumber, 0));
    class_unregister(virt_fft_class);
    class_destroy(virt_fft_class);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "virt_fft: Goodbye from the virtual FFT accelerator driver!\n");
}

module_init(virt_fft_init);
module_exit(virt_fft_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux driver for the virtual FFT accelerator");
MODULE_VERSION("0.1");
