#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <linux/types.h>

#define DEVICE_PATH "/dev/fft_acc"  // Device node path

// Define the ioctl commands as in the driver
#define GET_ID 0x00
#define GET_CTRL 0x10
#define GET_CFG0 0x20
#define GET_DATAOUT 0x30
#define GET_STATUS 0x40

#define SET_CTRL 0x50
#define SET_CFG0 0x60
#define SET_DATAIN 0x70

#define M_PI 3.14

// Helper function to read the STATUS register
u32 read_status(int fd) {
    u32 status;
    if (ioctl(fd, GET_STATUS, &status) < 0) {
        perror("Failed to read STATUS register");
        exit(EXIT_FAILURE);
    }
    return status;
}

// Helper function to wait for a specific status flag
void wait_for_status(int fd, u32 mask, u32 expected) {
    u32 status;
    do {
        status = read_status(fd);
    } while ((status & mask) != expected);
}

// Helper function to set a specific flag in a register
int set_config_flag(int fd, u32 flag, u32 mask) {
    int ret;
    u32 value;
    u32 result;

    ret = ioctl(fd, GET_CFG0, &result);
    if (ret < 0) {
        perror("Failed to read config register");
        close(fd);
        return EXIT_FAILURE;
    }

    value = (result & ~mask) | (flag & mask);
    ret = ioctl(fd, SET_CFG0, &value);
    if (ret < 0) {
        perror("Failed to set config register flag");
        close(fd);
        return EXIT_FAILURE;
    }
}

// Helper function to set a specific flag in a register
int set_control_flag(int fd, u32 flag, u32 mask) {
    int ret;
    u32 value;
    u32 result;

    ret = ioctl(fd, GET_CTRL, &result);
    if (ret < 0) {
        perror("Failed to read control register");
        close(fd);
        return EXIT_FAILURE;
    }

    value = (result & ~mask) | (flag & mask);
    ret = ioctl(fd, SET_CTRL, &value);
    if (ret < 0) {
        perror("Failed to set control register flag");
        close(fd);
        return EXIT_FAILURE;
    }
}

int main() {
    int i;
    int fd;
    int ret;
    u32 value;
    u32 samples[128];

    for (int i = 0; i < 128; i++) {
        samples[i] = (u32)(sin(2 * M_PI * i / 128) * 1000);
    }

    // Open the device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return EXIT_FAILURE;
    }

    // Step 1: Reset the device by deasserting EN
    value = 0x0;  // EN = 0
    ret = set_control_flag(fd, 0x0, 0x1);
    if (ret < 0) {
        perror("Failed to reset the device");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Device reset complete.\n");

    // Wait for READY flag
    wait_for_status(fd, 0x1, 0x1);  // Wait for READY = 1
    printf("Device is ready.\n");

    // Step 2: Configure the device
    value = (0x0 << 0) | (0x3 << 1);  
    ret = set_config_flag(fd, value, 0x7); // SMODE = 0 (32-bit samples), NSAMPLES = 0x7 (128 samples)
    ret = ioctl(fd, SET_CFG0, &value);
    if (ret < 0) {
        perror("Failed to configure the device");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Device configured: SMODE = 0 (32-bit), NSAMPLES = 1 (16 samples).\n");

    // Step 3: Load samples
    value = 0x1;  // EN = 1
    ret = set_control_flag(fd, 0x1, 0x1);
    if (ret < 0) {
        perror("Failed to enable the device");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Device enabled for loading samples.\n");

    for (i = 0; i < 128; i++) {
        u32 sample = samples[i];  // Example sample value
        ret = ioctl(fd, SET_DATAIN, &sample);
        if (ret < 0) {
            perror("Failed to write sample to DATAIN");
            close(fd);
            return EXIT_FAILURE;
        }

        value = 0x1 << 2;  // SAM = 1
        ret = set_control_flag(fd, value, value);
        if (ret < 0) {
            perror("Failed to assert SAM");
            close(fd);
            return EXIT_FAILURE;
        }

        // Wait for SCPLT flag
        wait_for_status(fd, 0x4, 0x4);  // Wait for SCPLT = 1
        printf("Sample %d loaded.\n", i);
    }

    // Step 4: Process the samples
    value = 0x1 << 3;  // PRC = 1
    ret = set_control_flag(fd, value, value);
    if (ret < 0) {
        perror("Failed to start processing");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Processing started.\n");

    // Wait for PCPLT flag
    wait_for_status(fd, 0x8, 0x8);  // Wait for PCPLT = 1
    printf("Processing complete.\n");

    // Step 5: Read the results
    value = 0x0;  // EN = 0
    ret = set_control_flag(fd, value, 0x1);
    if (ret < 0) {
        perror("Failed to disable the device");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Device disabled for reading results.\n");

    i = 0;
    while (read_status(fd) & 0x10) { // check EMPTY flag
        value = 0x1 << 2;  // SAM = 1
        ret = set_control_flag(fd, value, value);
        if (ret < 0) {
            perror("Failed to assert SAM");
            close(fd);
            return EXIT_FAILURE;
        }

        // Wait for SCPLT flag
        wait_for_status(fd, 0x4, 0x4);  // Wait for SCPLT = 1

        u32 result[128];
        ret = ioctl(fd, GET_DATAOUT, &result[i]);
        if (ret < 0) {
            perror("Failed to read DATAOUT");
            close(fd);
            return EXIT_FAILURE;
        }
        printf("Result %d: 0x%x\n", i, result[i]);
        i++;
    }

    // Close the device file
    close(fd);

    return EXIT_SUCCESS;
}