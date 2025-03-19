#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>

#define FFT_DEVICE "/dev/virt-fft-acc"  // Device file
#define MAP_SIZE 0x200

// Register Offsets
#define DEVID    0x000
#define CTRL     0x001
#define CFG0     0x002
#define DATAIN   0x003
#define DATAOUT  0x004
#define STATUS   0x005

// Control Register Fields
#define EN_FIELD  (1U << 0)
#define IEN_FIELD (1U << 1)
#define SAM_FIELD (1U << 2)
#define PRC_FIELD (1U << 3)

// Config Register Fields
#define SMODE_FIELD  (1U << 0)
#define NSAMPLES_FIELD (15U << 1)

#define SMODE_16 (0U << 0)
#define SMODE_32 (1U << 0)

#define NSAMPLES_16 (0U << 1)
#define NSAMPLES_32 (1U << 1)
#define NSAMPLES_64 (2U << 1)
#define NSAMPLES_128 (3U << 1)
#define NSAMPLES_256 (4U << 1)
#define NSAMPLES_512 (5U << 1)
#define NSAMPLES_1024 (6U << 1)
#define NSAMPLES_2048 (7U << 1)

// Status Register Fields
#define READY_FIELD  (1U << 0)
#define SCPLT_FIELD  (1U << 1)
#define PCPLT_FIELD  (1U << 2)
#define FULL_FIELD   (1U << 3)
#define EMPTY_FIELD  (1U << 4)

// FFT Sample Count
#define SAMPLES_COUNT 2048

typedef struct {
    int fd;
    volatile uint32_t *regs;
} FFTDevice;

// Open and map the device
int fft_init(FFTDevice *dev) {
    dev->fd = open(FFT_DEVICE, O_RDWR | O_SYNC);
    if (dev->fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    dev->regs = (volatile uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, 0);
    if (dev->regs == MAP_FAILED) {
        perror("Failed to mmap");
        close(dev->fd);
        return -1;
    }

    return 0;
}

// Close the device
void fft_close(FFTDevice *dev) {
    munmap((void *)dev->regs, MAP_SIZE);
    close(dev->fd);
}

// Trigger Reset process
void fft_reset(FFTDevice *dev) {
    dev->regs[CTRL] &= ~EN_FIELD;  // Disable device
    while (!(dev->regs[STATUS] & READY_FIELD));  // Wait for ready status
}

// Load sample data into FFT accelerator
void fft_setup(FFTDevice *dev, int n_samples, int s_length) {
    dev->regs[CFG0] |= s_length;  // Enable s_length bit sampling
    dev->regs[CFG0] |= n_samples;  // Set n_samples samples 
    dev->regs[CTRL] |= EN_FIELD;  // Enable device
}

// Load sample data into FFT accelerator
void fft_load_samples(FFTDevice *dev, uint32_t *samples, int count) {
    dev->regs[CTRL] |= EN_FIELD;  // Enable device

    for (int i = 0; i < count; i++) {
        dev->regs[DATAIN] = samples[i];
        dev->regs[CTRL] |= SAM_FIELD;  // Sample Data
        while (!(dev->regs[STATUS] & SCPLT_FIELD));  // Wait for sample completion
    }
}

// Trigger FFT processing
void fft_start_processing(FFTDevice *dev) {
    dev->regs[CTRL] |= PRC_FIELD;  // Start processing
    while (!(dev->regs[STATUS] & PCPLT_FIELD));  // Wait for completion
    dev->regs[CTRL] &= ~EN_FIELD;  // Disanable device
}

// Retrieve FFT results
void fft_get_results(FFTDevice *dev, uint32_t *output, int count) {
    int i = 0;
    while (!(dev->regs[STATUS] & EMPTY_FIELD)) {
        dev->regs[CTRL] |= SAM_FIELD;  // Request next sample
        output[i] = dev->regs[DATAOUT];
        while (!(dev->regs[STATUS] & SCPLT_FIELD));  // Wait for download completion
        i++;
    }
}

// Test function
void fft_test() {
    FFTDevice dev;
    if (fft_init(&dev) != 0) {
        return;
    }

    uint32_t samples[SAMPLES_COUNT];
    uint32_t output[SAMPLES_COUNT];

    // Generate sample sine wave
    for (int i = 0; i < SAMPLES_COUNT; i++) {
        samples[i] = (uint32_t)(sin(2 * M_PI * i / SAMPLES_COUNT) * 1000);
    }

    fft_reset(&dev);

    fft_setup(&dev, SMODE_32, NSAMPLES_2048);

    printf("Loading samples...\n");
    fft_load_samples(&dev, samples, SAMPLES_COUNT);

    printf("Starting FFT processing...\n");
    fft_start_processing(&dev);

    printf("Retrieving FFT results...\n");
    fft_get_results(&dev, output, SAMPLES_COUNT);

    // Print first 10 results
    for (int i = 0; i < 10; i++) {
        printf("FFT Output[%d]: %u\n", i, output[i]);
    }

    fft_close(&dev);
}

int main() {
    fft_test();
    return 0;
}
