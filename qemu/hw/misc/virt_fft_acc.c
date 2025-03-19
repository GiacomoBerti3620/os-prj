/**
 * VIRTUAL FFT Accelerator Device
 */

// Cooley-Tukey FFT
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#define SAMPLES_COUNT_MAX 2048

// QEMU Device implementation
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/bitops.h"
#include "qemu/log.h"
#include "hw/irq.h" // For qemu_set_irq


#define TYPE_VIRT_FFT_ACC          "virt-fft-acc"
#define VIRT_FFT_ACC(obj)          OBJECT_CHECK(VirtFftAccState, (obj), TYPE_VIRT_FFT_ACC)

/* Register map */
#define DEVID           0x000
#define ID              0xfacecafe

#define CTRL            0x001
#define EN_FIELD        ((uint32_t)1U << 0)
#define IEN_FIELD       ((uint32_t)1U << 1)
#define SAM_FIELD       ((uint32_t)1U << 2)
#define PRC_FIELD       ((uint32_t)1U << 3)

#define CFG0            0x002
#define SMODE_FIELD     ((uint32_t)1U << 0)
#define NSAMPLES_MASK   0x000e
#define NSAMPLES_SH     0x1

#define DATAIN          0x003
#define DATAIN_H_MASK   0xff00
#define DATAIN_H_SH     0x8
#define DATAIN_L_MASK   0x00ff
#define DATAIN_L_SH     0x0

#define DATAOUT         0x004

#define STATUS          0x005
#define READY_FIELD     ((uint32_t)1U << 0)
#define SCPLT_FIELD     ((uint32_t)1U << 1)
#define PCPLT_FIELD     ((uint32_t)1U << 2)
#define FULL_FIELD      ((uint32_t)1U << 3)
#define EMPTY_FIELD     ((uint32_t)1U << 4)

// Number of samples processed max (actual number set in NSAMPLES)
#define SAMPLES_COUNT   2048


// Struct representing device
typedef struct {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq;

    // Registers
    uint32_t id;
    uint32_t ctrl;
    uint32_t cfg;
    uint32_t datain;
    uint32_t dataout;
    uint32_t status;

    // Sample/Download indices
    uint16_t sampleIdx;
    uint16_t resultIdx;

    // Samples vectors
    uint32_t samplesInOut[SAMPLES_COUNT];

    // Number of samples selected
    uint16_t nSamples;
} VirtFftAccState;


// ------------------- FFT ALOGITHM START -------------------

// Cooley-Tukey based fft (https://it.wikipedia.org/wiki/Trasformata_di_Fourier_veloce)
// Function to reorder the complex array based on the reverse index
// Only real part is reordered since imaginary is just zeros
static void sort(int *f_real, int N) {
    int f2_real[SAMPLES_COUNT_MAX];

    for(int i = 0; i < N; i++) {
        // Calculate reverse index
        int j, rev = 0;
        for(j = 1; j <= log2(N); j++) {
            if(i & (1 << (int)(log2(N) - j)))
                rev |= 1 << (j - 1);
        }

        f2_real[i] = f_real[rev];
    }

    // Copy ordered arrays
    for(int j = 0; j < N; j++) {
        f_real[j] = f2_real[j];
    }
}

// Function to perform the Cooley-Tukey FFT
static void FFT(int *f_real, int *f_imag, int N) {
    // Create real and imaginary vectors
    double W_real[SAMPLES_COUNT_MAX / 2 * sizeof(double)];
    double W_imag[SAMPLES_COUNT_MAX / 2 * sizeof(double)];

    // Order arrays
    sort(f_real, N);

    // Compute complex numbers twiddle factors
    W_real[1] = cos(-2. * M_PI / N);
    W_imag[1] = sin(-2. * M_PI / N);
    W_real[0] = 1.0;
    W_imag[0] = 0.0;

    for(int i = 2; i < N / 2; i++) {
        W_real[i] = W_real[1] * W_real[i - 1] - W_imag[1] * W_imag[i - 1];
        W_imag[i] = W_imag[1] * W_real[i - 1] + W_real[1] * W_imag[i - 1];
    }

    int n = 1;
    int a = N / 2;
    for(int j = 0; j < log2(N); j++) {
        for(int i = 0; i < N; i++) {
            if(!(i & n)) {
                int temp_real = f_real[i];
                int temp_imag = f_imag[i];
                int temp_real2 = f_real[i + n];
                int temp_imag2 = f_imag[i + n];

                double W_real_val = W_real[(i * a) % (n * a)];
                double W_imag_val = W_imag[(i * a) % (n * a)];

                f_real[i] = temp_real + W_real_val * temp_real2 - W_imag_val * temp_imag2;
                f_imag[i] = temp_imag + W_real_val * temp_imag2 + W_imag_val * temp_real2;

                f_real[i + n] = temp_real - W_real_val * temp_real2 + W_imag_val * temp_imag2;
                f_imag[i + n] = temp_imag - W_real_val * temp_imag2 - W_imag_val * temp_real2;
            }
        }
        n *= 2;
        a = a / 2;
    }
}

// ------------------- FFT ALOGITHM END -------------------

// Just a wrapper around the Cooley-Tukey FFT
static void computeFFT(VirtFftAccState *s)
{
    // Real and imaginary parts of signal
    int wave_samples_real[SAMPLES_COUNT_MAX];
    int wave_samples_imag[SAMPLES_COUNT_MAX];

    // Copy the real values from the original sample
    for (int i = 0; i < s->nSamples; i++) {
        wave_samples_real[i] = s->samplesInOut[i];
        wave_samples_imag[i] = 0;  // Imaginary part is 0 initially
    }

    // Number of elements of vector is guaranteed to be power of two
    FFT(wave_samples_real, wave_samples_imag, s->nSamples);

    // Compute magnitudes
    for (int i = 0; i < s->nSamples; i++)
        s->samplesInOut[i] = sqrt(pow(wave_samples_real[i], 2) + pow(wave_samples_imag[i], 2));

    // Find max and min
    int min_val = s->samplesInOut[0];
    int max_val = 0;

    for (int i = 1; i < s->nSamples; i++) {
        if (s->samplesInOut[i] < min_val) {
            min_val = s->samplesInOut[i];
        }
        if (s->samplesInOut[i] > max_val) {
            max_val = s->samplesInOut[i];
        }
    }

    // Scale all samples
    for (int i = 0; i < s->nSamples; i++)
    {
        if (max_val == min_val)
            s->samplesInOut[i] = pow(2, (s->cfg & SMODE_FIELD)+4-1);
        else
            s->samplesInOut[i] = (float)(s->samplesInOut[i] - min_val) / (max_val - min_val) * (float)pow(2, (s->cfg & SMODE_FIELD)+4);
    }

    return;
}

// Set IRQ method
static void virt_fft_acc_set_irq(VirtFftAccState *s, int irq)
{
    // Set interrupt status
    //s->status = (s->status & ~irq) | irq;
    s->status = s->status | irq;
    
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

static void virt_fft_acc_sample_data(VirtFftAccState *s)
{
    // Depending on sample size chosen
    if (s->cfg & SMODE_FIELD)
    {
        // 16-bit samples

        // First sample in (DATAIN_LOW)
        s->samplesInOut[s->sampleIdx] = s->datain & 0x00ff;
        s->sampleIdx++;

        // Second sample in (DATAIN_HIGH)
        s->samplesInOut[s->sampleIdx] = s->datain & 0xff00;
        s->sampleIdx++;
    }
    else
    {
        // 32-bit samples
        s->samplesInOut[s->sampleIdx] = s->datain;
        s->sampleIdx++;
    }
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
        // Deassert interrupt
        virt_fft_acc_clr_irq(s);
        
        // Save return value
        uint64_t status = s->status;

        // Reset clear-on-read fields
        s->status = s->status & (EMPTY_FIELD | FULL_FIELD);

        return status;

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

    switch (offset) {
    case CTRL:
        // Copy only EN fields
        s->ctrl = (int)value & (EN_FIELD | IEN_FIELD);

        // Load data or process
        if (value & SAM_FIELD)
        {
            // Proceed according to device status
            if (s->ctrl & EN_FIELD)
            {
                // Sample datain
                if (s->sampleIdx < s->nSamples)
                {
                    virt_fft_acc_sample_data(s);
                }

                // Interrupt when op complete and if full
                virt_fft_acc_set_irq(s, SCPLT_FIELD | (s->sampleIdx == s->nSamples ? FULL_FIELD : 0));
    
                // Feedback
                if (s->sampleIdx == s->nSamples)
                    printf("All %0d samples loaded, full and ready to process", s->nSamples);
            }
            else
            {
                // Download dataout
                if (s->resultIdx < s->nSamples/2)
                {
                    s->dataout = s->samplesInOut[s->resultIdx];
                    s->resultIdx++;
                }

                // Interrupt when op complete
                virt_fft_acc_set_irq(s, SCPLT_FIELD | (s->resultIdx == s->nSamples/2 ? EMPTY_FIELD : 0));

                // Feedback
                if (s->resultIdx < s->nSamples/2)
                    printf("All %0d samples downloaded, empty", s->nSamples/2);
            }
        }
        else if (value & PRC_FIELD)
        {
            // Run FFT algorithm
            computeFFT(s);

            // Interrupt when op complete
            virt_fft_acc_set_irq(s, PCPLT_FIELD);
        }

        // Reset operations
        if (s->ctrl & EN_FIELD){
            // Enabled, reset download counter
            s->resultIdx = 0;
        }
        else {
            // Reset upload counter
            s->sampleIdx = 0;

            // Set status register
            s->status = s->status | READY_FIELD;
        }
        break;

    case CFG0:
        // Copy fields
        s->cfg = (int)value;

        // Set samples count
        s->nSamples = (uint16_t) pow(2, (double) (((s->cfg & NSAMPLES_MASK) >> NSAMPLES_SH)+4));
        break;

    case DATAIN:
        // Copy value to register
        s->datain = value;
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

    s->id = ID; 
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