#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/virt_fft_acc"
#define VIRT_FFT_IOCTL_RESET _IO('V', 1)
#define VIRT_FFT_IOCTL_CONFIGURE _IOW('V', 2, uint32_t)
#define VIRT_FFT_IOCTL_LOAD_SAMPLE _IOW('V', 3, uint32_t)
#define VIRT_FFT_IOCTL_PROCESS _IO('V', 4)
#define VIRT_FFT_IOCTL_READ_RESULT _IOR('V', 5, uint32_t)

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }

    // Reset the accelerator
    if (ioctl(fd, VIRT_FFT_IOCTL_RESET) < 0) {
        perror("Failed to reset the accelerator");
        close(fd);
        return -1;
    }
    printf("Accelerator reset successfully\n");

    // Configure the device (example: set SMODE to 0 for 32-bit samples)
    uint32_t cfg_value = 0x0; // SMODE = 0, NSAMPLES = default
    if (ioctl(fd, VIRT_FFT_IOCTL_CONFIGURE, &cfg_value) < 0) {
        perror("Failed to configure the device");
        close(fd);
        return -1;
    }
    printf("Device configured successfully\n");

    // Load samples (example: load 3 samples)
    uint32_t samples[] = {0x12345678, 0x9ABCDEF0, 0x13579BDF};
    for (int i = 0; i < 3; i++) {
        if (ioctl(fd, VIRT_FFT_IOCTL_LOAD_SAMPLE, &samples[i]) < 0) {
            perror("Failed to load sample");
            close(fd);
            return -1;
        }
        printf("Sample %d loaded successfully\n", i + 1);
    }

    // Process the samples
    if (ioctl(fd, VIRT_FFT_IOCTL_PROCESS) < 0) {
        perror("Failed to process samples");
        close(fd);
        return -1;
    }
    printf("Samples processed successfully\n");

    // Read the results
    uint32_t result;
    for (int i = 0; i < 3; i++) {
        if (ioctl(fd, VIRT_FFT_IOCTL_READ_RESULT, &result) < 0) {
            perror("Failed to read result");
            close(fd);
            return -1;
        }
        printf("Result %d: 0x%x\n", i + 1, result);
    }

    close(fd);
    return 0;
}