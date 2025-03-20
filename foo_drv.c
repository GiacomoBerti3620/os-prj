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
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/hw.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"

#define TYPE_PCI_CUSTOM_DEVICE "virt-fft-acc"

/* Register map */
#define DEVID 0x0
#define ID 0xfacecafe

#define CTRL 0x1
#define EN_FIELD ((uint32_t)1U << 0)
#define IEN_FIELD ((uint32_t)1U << 1)
#define SAM_FIELD ((uint32_t)1U << 2)
#define PRC_FIELD ((uint32_t)1U << 3)

#define CFG0 0x2
#define SMODE_FIELD ((uint32_t)1U << 0)
#define NSAMPLES_MASK 0x000e
#define NSAMPLES_SH 0x1

#define DATAIN 0x3
#define DATAIN_H_MASK 0xff00
#define DATAIN_H_SH 0x8
#define DATAIN_L_MASK 0x00ff
#define DATAIN_L_SH 0x0

#define DATAOUT 0x4

#define STATUS 0x5
#define READY_FIELD ((uint32_t)1U << 0)
#define SCPLT_FIELD ((uint32_t)1U << 1)
#define PCPLT_FIELD ((uint32_t)1U << 2)
#define FULL_FIELD ((uint32_t)1U << 3)
#define EMPTY_FIELD ((uint32_t)1U << 4)

typedef struct PcifftdevState PcifftdevState;

// This macro provides the instance type cast functions for a QOM type.
DECLARE_INSTANCE_CHECKER(PcifftdevState, PCIFFTDEV, TYPE_PCI_CUSTOM_DEVICE)

// struct defining/descring the state
// of the custom pci device.
struct PcifftdevState{
        PCIDevice pdev;
        MemoryRegion mmio_bar0;
        uint32_t bar0[6];
        // Sample/Download indices
        uint16_t sampleIdx;
        uint16_t resultIdx;

        // Samples vectors
        uint32_t samplesInOut[SAMPLES_COUNT_MAX];

        // Number of samples selected
        uint16_t nSamples;
    };


// ------------------- FFT ALOGITHM START -------------------

// Cooley-Tukey based fft (https://it.wikipedia.org/wiki/Trasformata_di_Fourier_veloce)
// Function to reorder the complex array based on the reverse index
// Only real part is reordered since imaginary is just zeros
static void sort(int *f_real, int N)
{
    int f2_real[SAMPLES_COUNT_MAX];

    for (int i = 0; i < N; i++)
    {
        // Calculate reverse index
        int j, rev = 0;
        for (j = 1; j <= log2(N); j++)
        {
            if (i & (1 << (int)(log2(N) - j)))
                rev |= 1 << (j - 1);
        }

        f2_real[i] = f_real[rev];
    }

    // Copy ordered arrays
    for (int j = 0; j < N; j++)
    {
        f_real[j] = f2_real[j];
    }
}

// Function to perform the Cooley-Tukey FFT
static void FFT(int *f_real, int *f_imag, int N)
{
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

    for (int i = 2; i < N / 2; i++)
    {
        W_real[i] = W_real[1] * W_real[i - 1] - W_imag[1] * W_imag[i - 1];
        W_imag[i] = W_imag[1] * W_real[i - 1] + W_real[1] * W_imag[i - 1];
    }

    int n = 1;
    int a = N / 2;
    for (int j = 0; j < log2(N); j++)
    {
        for (int i = 0; i < N; i++)
        {
            if (!(i & n))
            {
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
static void computeFFT(PcifftdevState *s)
{
    // Real and imaginary parts of signal
    int wave_samples_real[SAMPLES_COUNT_MAX];
    int wave_samples_imag[SAMPLES_COUNT_MAX];

    // Copy the real values from the original sample
    for (int i = 0; i < s->nSamples; i++)
    {
        wave_samples_real[i] = s->samplesInOut[i];
        wave_samples_imag[i] = 0; // Imaginary part is 0 initially
    }

    // Number of elements of vector is guaranteed to be power of two
    FFT(wave_samples_real, wave_samples_imag, s->nSamples);

    // Compute magnitudes
    for (int i = 0; i < s->nSamples; i++)
        s->samplesInOut[i] = sqrt(pow(wave_samples_real[i], 2) + pow(wave_samples_imag[i], 2));

    // Find max and min
    int min_val = s->samplesInOut[0];
    int max_val = 0;

    for (int i = 1; i < s->nSamples; i++)
    {
        if (s->samplesInOut[i] < min_val)
        {
            min_val = s->samplesInOut[i];
        }
        if (s->samplesInOut[i] > max_val)
        {
            max_val = s->samplesInOut[i];
        }
    }

    // Scale all samples
    for (int i = 0; i < s->nSamples; i++)
    {
        if (max_val == min_val)
            s->samplesInOut[i] = pow(2, (s->cfg & SMODE_FIELD) + 4 - 1);
        else
            s->samplesInOut[i] = (float)(s->samplesInOut[i] - min_val) / (max_val - min_val) * (float)pow(2, (s->cfg & SMODE_FIELD) + 4);
    }

    return;
}


static void virt_fft_acc_sample_data(PcifftdevState *s)
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
static uint64_t pcifftdev_bar0_mmio_write(void *opaque, hwaddr offset, unsigned size)
{
    // Cast to device struct
    PcifftdevState *s = (PcifftdevState *)opaque;

    // Return registers content
    switch (offset)
    {
    case DEVID:
        return s->bar0[0];
    case CTRL:
        return s->bar0[1];
    case CFG0:
        return s->bar0[2];
    case DATAOUT:
        return s->bar0[4];
    case STATUS:

        // Save return value
        uint32_t status = s->bar0[5];

        // Reset clear-on-read fields
        s->bar0[5] = s->status & (EMPTY_FIELD | FULL_FIELD);

        return status;

    default:
        break;
    }

    return 0;
}

// Memory write implementation
static void pcifftdev_bar0_mmio_read(void *opaque, hwaddr offset, uint64_t value,
                               unsigned size)
{
    // Cast to device struct
    PcifftdevState *s = (PcifftdevState *)opaque;

    switch (offset)
    {
    case CTRL:
        // Copy only EN fields
        s->bar0[1] = (int)value & (EN_FIELD | IEN_FIELD);

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

                // Feedback
                if (s->sampleIdx == s->nSamples)
                    printf("All %0d samples loaded, full and ready to process", s->nSamples);
            }
            else
            {
                // Download dataout
                if (s->resultIdx < s->nSamples / 2)
                {
                    s->bar0[4] = s->samplesInOut[s->resultIdx];
                    s->resultIdx++;
                }

                // Feedback
                if (s->resultIdx == s->nSamples / 2)
                    printf("All %0d samples downloaded, empty", s->nSamples / 2);
            }
        }
        else if (value & PRC_FIELD)
        {
            // Run FFT algorithm
            computeFFT(s);
        }

        // Reset operations
        if (s->ctrl & EN_FIELD)
        {
            // Enabled, reset download counter
            s->resultIdx = 0;
        }
        else
        {
            // Reset upload counter
            s->sampleIdx = 0;

            // Set status register
            s->bar0[5] = s->bar0[5] | READY_FIELD;
        }
        break;

    case CFG0:
        // Copy fields
        s->bar0[2] = (int)value;

        // Set samples count
        s->nSamples = (uint16_t)pow(2, (double)(((s->bar0[2] & NSAMPLES_MASK) >> NSAMPLES_SH) + 4));
        break;

    case DATAIN:
        // Copy value to register
        s->bar0[3] = value;
        break;

    default:
        break;
    }
}

///ops for the Memory Region.
static const MemoryRegionOps pcifftdev_bar0_mmio_ops = {
	.read = pcifftdev_bar0_mmio_read,
	.write = pcifftdev_bar0_mmio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
		.min_access_size = 4,
		.max_access_size = 4,
	},
	.impl = {
		.min_access_size = 4,
		.max_access_size = 4,
	},

};

//implementation of the realize function.
static void pci_pcifftdev_realize(PCIDevice *pdev, Error **errp)
{
	PcifftdevState *pcifftdev = PCIFFTDEV(pdev);
	uint8_t *pci_conf = pdev->config;

	pci_config_set_interrupt_pin(pci_conf, 1);

	///initial configuration of devices registers.
	memset(pcifftdev->bar0, 0, 24);
	pcifftdev->bar0[0] = 0xcafeaffe;

	// Initialize an I/O memory region(pcifftdev->mmio). 
	// Accesses to this region will cause the callbacks 
	// of the pcifftdev_mmio_ops to be called.
	memory_region_init_io(&pcifftdev->mmio_bar0, OBJECT(pcifftdev), &pcifftdev_bar0_mmio_ops, pcifftdev, "pcifftdev-mmio", 24);
	// registering the pdev and all of the above configuration 
	// (actually filling a PCI-IO region with our configuration.
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &pcifftdev->mmio_bar0);
}

// uninitializing functions performed.
static void pci_pcifftdev_uninit(PCIDevice *pdev)
{
	return;
}


///initialization of the device
static void pcifftdev_instance_init(Object *obj)
{
	return ;
}

static void pcifftdev_class_init(ObjectClass *class, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

	//definition of realize func().
	k->realize = pci_pcifftdev_realize;
	//definition of uninit func().
	k->exit = pci_pcifftdev_uninit;
	k->vendor_id = PCI_VENDOR_ID_QEMU;
	k->device_id = 0xfacecafe; //our device id, 'beef' hexadecimal
	k->revision = 0x10;
	k->class_id = PCI_CLASS_OTHERS;

	/**
	 * set_bit - Set a bit in memory
	 * @nr: the bit to set
	 * @addr: the address to start counting from
	 */
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static void pci_custom_device_register_types(void)
{
	static InterfaceInfo interfaces[] = {
		{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
		{ },
	};
	static const TypeInfo custom_pci_device_info = {
		.name          = TYPE_PCI_CUSTOM_DEVICE,
		.parent        = TYPE_PCI_DEVICE,
		.instance_size = sizeof(PcifftdevState),
		.instance_init = pcifftdev_instance_init,
		.class_init    = pcifftdev_class_init,
		.interfaces = interfaces,
	};
	//registers the new type.
	type_register_static(&custom_pci_device_info);
}

type_init(pci_custom_device_register_types)