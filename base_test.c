#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <stdint.h>

#define DEVICE_PATH "/dev/fft_acc"  // Assuming the device node is created at /dev/fft_accdev

// Define the ioctl commands as in the driver
#define GET_ID 0x00
#define GET_CTRL 0x10
#define GET_CFG0 0x20
#define GET_DATAOUT 0x30
#define GET_STATUS 0x40

#define SET_CTRL 0x50
#define SET_CFG0 0x60
#define SET_DATAIN 0x70

typedef uint32_t u32;

int main() {
    int fd;
    int ret;
    u32 value;

    // Open the device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return EXIT_FAILURE;
    }

    // Test GET_ID
    ret = ioctl(fd, GET_ID, &value);
    if (ret < 0) {
        perror("GET_ID ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("GET_ID: 0x%x\n", value);

    // Test GET_CTRL
    ret = ioctl(fd, GET_CTRL, &value);
    if (ret < 0) {
        perror("GET_CTRL ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("GET_CTRL: 0x%x\n", value);

    // Test GET_CFG0
    ret = ioctl(fd, GET_CFG0, &value);
    if (ret < 0) {
        perror("GET_CFG0 ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("GET_CFG0: 0x%x\n", value);

    // Test GET_DATAOUT
    ret = ioctl(fd, GET_DATAOUT, &value);
    if (ret < 0) {
        perror("GET_DATAOUT ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("GET_DATAOUT: 0x%x\n", value);

    // Test GET_STATUS
    ret = ioctl(fd, GET_STATUS, &value);
    if (ret < 0) {
        perror("GET_STATUS ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("GET_STATUS: 0x%x\n", value);

    // Test SET_CTRL
    value = 0x12345678;  // Example value to set
    ret = ioctl(fd, SET_CTRL, &value);
    if (ret < 0) {
        perror("SET_CTRL ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("SET_CTRL: 0x%x\n", value);

    // Test SET_CFG0
    value = 0x87654321;  // Example value to set
    ret = ioctl(fd, SET_CFG0, &value);
    if (ret < 0) {
        perror("SET_CFG0 ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("SET_CFG0: 0x%x\n", value);

    // Test SET_DATAIN
    value = 0xABCDEF12;  // Example value to set
    ret = ioctl(fd, SET_DATAIN, &value);
    if (ret < 0) {
        perror("SET_DATAIN ioctl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("SET_DATAIN: 0x%x\n", value);

    // Close the device file
    close(fd);

    return EXIT_SUCCESS;
}