#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dmaengine.h>

#define DEVNR 64
#define DEVNRNAME "fft_acc"

#define VID 0x1234
#define DID 0xcafe

#define OFFS_ID 0x0
#define OFF_CTRL 0x4
#define OFFS_CFG0 0x8
#define OFFS_DATAIN 0xc
#define OFFS_DATAOUT 0x10
#define OFFS_STATUS 0x14

#define GET_ID 0X00
#define GET_CTRL 0X10
#define GET_CFG0 0X20
#define GET_DATAOUT 0X30
#define GET_STATUS 0X40

#define SET_CTRL 0X50
#define SET_CFG0 0X60
#define SET_DATAIN 0X70

struct fft_accdev
{
    struct pci_dev *pdev;
    void __iomem *ptr_bar0;
} mydev;

static long int fft_acc_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
    struct fft_accdev *fft_acc = &mydev;
    u32 val;

    switch (cmd)
    {
    case GET_ID:
        val = ioread32(fft_acc->ptr_bar0 + OFFS_ID);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_CTRL:
        val = ioread32(fft_acc->ptr_bar0 + OFF_CTRL);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_CFG0:
        val = ioread32(fft_acc->ptr_bar0 + OFFS_CFG0);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_DATAOUT:
        val = ioread32(fft_acc->ptr_bar0 + OFFS_DATAOUT);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_STATUS:
        val = ioread32(fft_acc->ptr_bar0 + OFFS_STATUS);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case SET_CTRL:
        if (0 != copy_from_user(&val, (u32 *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, fft_acc->ptr_bar0 + OFF_CTRL);
        return 0;
    case SET_CFG0:
        if (0 != copy_from_user(&val, (u32 *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, fft_acc->ptr_bar0 + OFFS_CFG0);
        return 0;
    case SET_DATAIN:
        if (0 != copy_from_user(&val, (u32 *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, fft_acc->ptr_bar0 + OFFS_DATAIN);
        return 0;

    default:
        return -EINVAL;
    }
}

static struct file_operations fops = {
    .unlocked_ioctl = fft_acc_ioctl,
};
//.mmap = fft_acc_mmap,

static struct pci_device_id fft_acc_ids[] = {
    {PCI_DEVICE(VID, DID)},
    {},
};
MODULE_DEVICE_TABLE(pci, fft_acc_ids);

static int fft_acc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int status;
    void __iomem *ptr_bar0;
    mydev.pdev = pdev;

    status = register_chrdev(DEVNR, DEVNRNAME,  &fops);
    if (status < 0)
    {
        printk("fft_acc_drv - Error registering Device numbers\n");
        return status;
    }

    status = pcim_enable_device(pdev);
    if (status != 0)
    {
        printk("fft_acc_drv - Error enabling device\n");
        return -ENODEV;
    }

    mydev.ptr_bar0 = pcim_iomap(pdev, 0, pci_resource_len(pdev, 0));
    if (!mydev.ptr_bar0)
    {
        printk("fft_acc_drv - Error mapping BAR0\n");
        return -ENODEV;
    }

    return 0;
}

static void fft_acc_remove(struct pci_dev *pdev)
{
    printk("fft_acc_drv - Removing the device\n");
    unregister_chrdev(DEVNR, DEVNRNAME);
}

static struct pci_driver fft_acc_driver = {
    .name = "fft_acc",
    .probe = fft_acc_probe,
    .remove = fft_acc_remove,
    .id_table = fft_acc_ids,
};

module_pci_driver(fft_acc_driver);

MODULE_LICENSE("GPL");