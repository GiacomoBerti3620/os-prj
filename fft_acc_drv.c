#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dmaengine.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#define DEVNR 64
#define DEVNRNAME "fft_accdev"

#define VID 0x1234
#define DID 0xbeef

#define DMA_SRC 0x10
#define DMA_DST 0x18
#define DMA_CNT 0x20
#define DMA_CMD 0x28
#define DMA_RUN 1

#define GET_ID 0x00
#define GET_RAND 0x20
#define GET_INV 0x30
#define SET_INV 0x40
#define IRQ 0x50

struct fft_accdev
{
    struct pci_dev *pdev;
    void __iomem *ptr_bar0;
    struct list_head list;
    struct cdev cdev;
    dev_t dev_nr;
};

/* Global Variables */
LIST_HEAD(card_list);
static struct mutex lock;
static int card_count = 0;

static irqreturn_t fft_acc_irq_handler(int irq_nr, void *data)
{
    struct fft_accdev *fft_acc = (struct fft_accdev *)data;

    printk("fft_acc_drv - IRQ triggered!\n");
    
    return IRQ_HANDLED;
}


static int fft_acc_open(struct inode *inode, struct file *file)
{
    struct fft_accdev *fft_acc;
    dev_t dev_nr = inode->i_rdev;

    list_for_each_entry(fft_acc, &card_list, list)
    {
        if (fft_acc->dev_nr == dev_nr)
        {
            file->private_data = fft_acc;
            return 0;
        }
    }
    return -ENODEV;
}
/*
static ssize_t echo_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offs)
{
    char *buf;
    int not_copied, to_copy = (count + *offs < 4096) ? count : 4096 - *offs;
    struct echodev *echo = (struct echodev *)file->private_data;

    if (*offs >= pci_resource_len(echo->pdev, 1))
        return 0;

    buf = kmalloc(to_copy, GFP_ATOMIC);
    not_copied = copy_from_user(buf, user_buffer, to_copy);

    dma_transfer(echo, buf, to_copy, *offs, DMA_TO_DEVICE);

    kfree(buf);
    *offs += to_copy - not_copied;
    return to_copy - not_copied;
}

static ssize_t echo_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offs)
{
    char *buf;
    struct echodev *echo = (struct echodev *)file->private_data;
    int not_copied, to_copy = (count + *offs < pci_resource_len(echo->pdev, 1)) ? count : pci_resource_len(echo->pdev, 1) - *offs;

    if (to_copy == 0)
        return 0;

    buf = kmalloc(to_copy, GFP_ATOMIC);

    dma_transfer(echo, buf, to_copy, *offs, DMA_FROM_DEVICE);

    mdelay(5);
    not_copied = copy_to_user(user_buffer, buf, to_copy);

    kfree(buf);
    *offs += to_copy - not_copied;
    return to_copy - not_copied;
}
*/
static int fft_acc_mmap(struct file *file, struct vm_area_struct *vma)
{
    int status;
    struct fft_accdev *fft_acc = (struct fft_accdev *)file->private_data;

    vma->vm_pgoff = pci_resource_start(fft_acc->pdev, 1) >> PAGE_SHIFT;

    status = io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
    if (status)
    {
        printk("fft_acc_drv - Error allocating device number\n");
        return -status;
    }
    return 0;
}

static long int fft_acc_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
    struct fft_accdev *fft_acc = (struct fft_accdev *)file->private_data;
    u32 val;

    switch (cmd)
    {
    case GET_ID:
        val = ioread32(fft_acc->ptr_bar0 + 0x00);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_INV:
        val = ioread32(fft_acc->ptr_bar0 + 0x04);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case GET_RAND:
        val = ioread32(fft_acc->ptr_bar0 + 0x0C);
        return copy_to_user((u32 *)arg, &val, sizeof(val));
    case SET_INV:
        if (0 != copy_from_user(&val, (u32 *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, fft_acc->ptr_bar0 + 0x4);
        return 0;
    case IRQ:
        iowrite32(1, fft_acc->ptr_bar0 + 0x8);
        return 0;
    default:
        return -EINVAL;
    }
}

static struct file_operations fops = {
    .open = fft_acc_open,
    .mmap = fft_acc_mmap,
    .unlocked_ioctl = fft_acc_ioctl,
};

    //.read = fft_acc_read,
    //.write = fft_acc_write,

static struct pci_device_id fft_acc_ids[] = {
    {PCI_DEVICE(VID, DID)},
    {},
};
MODULE_DEVICE_TABLE(pci, fft_acc_ids);

static int fft_acc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int status, irq_nr;
    struct fft_accdev *fft_acc;

    fft_acc = devm_kzalloc(&pdev->dev, sizeof(struct fft_accdev), GFP_KERNEL);
    if (!fft_acc)
        return -ENOMEM;

    mutex_lock(&lock);
    cdev_init(&fft_acc->cdev, &fops);
    fft_acc->cdev.owner = THIS_MODULE;

    fft_acc->dev_nr = MKDEV(DEVNR, card_count++);
    status = cdev_add(&fft_acc->cdev, fft_acc->dev_nr, 1);
    if (status < 0)
    {
        printk("fft_acc_drv - Error adding cdev\n");
        return status;
    }

    list_add_tail(&fft_acc->list, &card_list);
    mutex_unlock(&lock);

    fft_acc->pdev = pdev;

    status = pcim_enable_device(pdev);
    if (status != 0)
    {
        printk("fft_acc_drv - Error enabling device\n");
        goto fdev;
    }

    pci_set_master(pdev);

    fft_acc->ptr_bar0 = pcim_iomap(pdev, 0, pci_resource_len(pdev, 0));
    if (!fft_acc->ptr_bar0)
    {
        printk("fft_acc_drv - Error mapping BAR0\n");
        status = -ENODEV;
        goto fdev;
    }

    pci_set_drvdata(pdev, fft_acc);

    status = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
    if (status != 1)
    {
        printk("fft_acc_drv - Error alloc_irq returned %d\n", status);
        status = -ENODEV;
        goto fdev;
    }

    irq_nr = pci_irq_vector(pdev, 0);
    printk("fft_acc_drv - IRQ Number: %d\n", irq_nr);

    status = devm_request_irq(&pdev->dev, irq_nr, fft_acc_irq_handler, 0,
                              "fft_acc-irq", fft_acc);
    if (status != 0)
    {
        printk("fft_acc_drv - Error requesting interrupt\n");
        goto fdev;
    }

    return 0;

fdev:
    /* Removing fft_acc from list is missing */
    cdev_del(&fft_acc->cdev);
    return status;
}

static void fft_acc_remove(struct pci_dev *pdev)
{
    struct fft_accdev *fft_acc = (struct fft_accdev *)pci_get_drvdata(pdev);
    printk("fft_acc_drv - Removing the device with Device Number %d:%d\n",
           MAJOR(fft_acc->dev_nr), MINOR(fft_acc->dev_nr));
    if (fft_acc)
    {
        mutex_lock(&lock);
        list_del(&fft_acc->list);
        mutex_unlock(&lock);
        cdev_del(&fft_acc->cdev);
    }
    pci_free_irq_vectors(pdev);
}

static struct pci_driver fft_acc_driver = {
    .name = "fft-acc-driver",
    .probe = fft_acc_probe,
    .remove = fft_acc_remove,
    .id_table = fft_acc_ids,
};

static int __init fft_acc_init(void)
{
    int status;
    dev_t dev_nr = MKDEV(DEVNR, 0);

    status = register_chrdev_region(dev_nr, MINORMASK + 1, DEVNRNAME);
    if (status < 0)
    {
        printk("fft_acc_drv - Error registering Device numbers\n");
        return status;
    }

    mutex_init(&lock);

    status = pci_register_driver(&fft_acc_driver);
    if (status < 0)
    {
        printk("fft_acc_drv - Error registering driver\n");
        unregister_chrdev_region(dev_nr, MINORMASK + 1);
        return status;
    }
    return 0;
}

static void __exit fft_acc_exit(void)
{
    dev_t dev_nr = MKDEV(DEVNR, 0);
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
    pci_unregister_driver(&fft_acc_driver);
}

module_init(fft_acc_init);
module_exit(fft_acc_exit);

MODULE_LICENSE("GPL");