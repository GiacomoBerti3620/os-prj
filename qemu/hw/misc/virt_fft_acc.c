/**
 * VIRTUAL FFT Accelerator Device
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/bitops.h"
#include "qemu/log.h"


#define TYPE_VIRT_FFT_ACC          "virt-fft_acc"
#define VIRT_FFT_ACC(obj)          OBJECT_CHECK(VirtFftAccState, (obj), TYPE_VIRT_FFT_ACC)

/* Register map */
#define DEVID           0x000
#define ID              0xfacecafe

#define CTRL            0x001
#define EN_FIELD        BIT(0)
#define IEN_FIELD       BIT(1)
#define SAM_FIELD       BIT(2)
#define PRC_FIELD       BIT(3)

#define CFG0            0x002
#define SMODE_FIELD     BIT(0)
#define NSAMPLES_MASK   0x000e
#define NSAMPLES_SH     0x1

#define DATAIN          0x003
#define DATAIN_H_MASK   0xff00
#define DATAIN_H_SH     0x8
#define DATAIN_L_MASK   0x00ff
#define DATAIN_L_SH     0x0

#define DATAOUT         0x004

#define STATUS          0x005
#define READY_FIELD     BIT(0)
#define SCPLT_FIELD     BIT(1)
#define PCPLT_FIELD     BIT(2)
#define FULL_FIELD      BIT(3)
#define EMPTY_FIELD     BIT(4)


// Struct representing device
typedef struct {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq;
    uint32_t id;
    uint32_t ctrl;
    uint32_t cfg;
    uint32_t datain;
    uint32_t dataout;
    uint32_t status;
    uint16_t sampleIdx;
    uint16_t resultIdx;
} VirtFftAccState;

// Set IRQ method
static void virt_fft_acc_set_irq(VirtFftAccState *s, int irq)
{
    // Set interrupt status
    // TODO Here?
    
    // Trigger interrupt if enabled
    if (s->ctrl & IEN_FIELD)
    {
        qemu_set_irq(s->irq, 1);
    }
}

// Clear IRQ method
static void virt_fft_acc_clr_irq(VirtFftAccState *s)
{
    // Always clear
    qemu_set_irq(s->irq, 0);
}

// Memory read implementation
static uint64_t virt_fft_acc_read(void *opaque, hwaddr offset, unsigned size)
{
    // Cast to device struct
    VirtFftAccState *s = (VirtFftAccState *)opaque;

    // Return registers content
    switch (offset) {
    case DEVID:
        return ID;
    case CTRL:
        return s->ctrl;
    case CFG0:
        return s->cfg;
    case DATAOUT:
        return s->dataout;
    case STATUS:
        virt_fft_acc_clr_irq(s);
        return s->status;
    default:
        break;
    }

    return 0;
}

// Memory write implementation
static void virt_fft_acc_write(void *opaque, hwaddr offset, uint64_t value,
                          unsigned size)
{
    // Cast to device struct
    VirtFftAccState *s = (VirtFftAccState *)opaque;



    // Operate according to enable bit
    if (s->ctrl & EN_FIELD)
    {
        // Enabled, can write datain and trigger new processing
    }
    else
    {
        // Disabled, can read data out
    }
    

    switch (offset) {
    case CTRL:
        // Copy fields
        s->init = (int)value;

        // Load data or process
        if
        break;
    case REG_CMD:
        s->cmd = (int)value;
        virt_fft_acc_set_irq(s, INT_BUFFER_DEQ);
        break;
    default:
        break;
    }
}

static const MemoryRegionOps virt_fft_acc_ops = {
    .read = virt_fft_acc_read,
    .write = virt_fft_acc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void virt_fft_acc_realize(DeviceState *d, Error **errp)
{
    VirtFftAccState *s = VIRT_FFT_ACC(d);
    SysBusDevice *sbd = SYS_BUS_DEVICE(d);

    memory_region_init_io(&s->iomem, OBJECT(s), &virt_fft_acc_ops, s,
                          TYPE_VIRT_FFT_ACC, 0x200);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    s->id = CHIP_ID; 
    s->init = 0;
}

static void virt_fft_acc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = virt_fft_acc_realize;
}

static const TypeInfo virt_fft_acc_info = {
    .name          = TYPE_VIRT_FFT_ACC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VirtFftAccState),
    .class_init    = virt_fft_acc_class_init,
};

static void virt_fft_acc_register_types(void)
{
    type_register_static(&virt_fft_acc_info);
}

type_init(virt_fft_acc_register_types)