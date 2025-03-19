#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>

#define DEVICE_FILE "/dev/fft_acc"
#define FFT_ACC_RESET     _IO('F', 0)
#define FFT_ACC_START     _IO('F', 1)
#define FFT_ACC_SET_CFG   _IOW('F', 2, uint32_t)
#define FFT_ACC_LOAD_DATA _IOW('F', 3, uint32_t)
#define FFT_ACC_GET_RESULT _IOR('F', 4, uint32_t)

#define NUM_SAMPLES 2048
#define PI 3.14

void generate_sine_wave(uint32_t *buffer, int bit_mode) {
    if (bit_mode == 32) {
		for (int i = 0; i < NUM_SAMPLES; i++) {
			double angle = 2.0 * PI * i / NUM_SAMPLES;
			double sine_value = sin(angle);
			buffer[i] = (uint32_t)((sine_value + 1.0) * (0x7FFFFFFF)); // Scale to 32-bit unsigned
		}
    } else {
		for (int i = 0; i < NUM_SAMPLES; i = i+2) {
			double angle0 = 2.0 * PI * i / NUM_SAMPLES;
			double angle1 = 2.0 * PI * i+1 / NUM_SAMPLES;
			double sine_value0 = sin(angle0);
			double sine_value1 = sin(angle1);
            uint16_t high = (uint16_t)((int)((sine_value0 + 1.0) * 0x7FFF) & 0xFFFF);
			uint16_t low = (uint16_t)((int)((sin_value1 + 1.0) * 0x7FFF) & 0xFFFF);
            buffer[i] = (high << 16) | low;
		}
    }
}

void load_samples(int fd, uint32_t *sine_wave, int bit_mode) {
    ioctl(fd, FFT_ACC_RESET);
    uint32_t cfg = (bit_mode == 32) ? 0x6 : 0x7; // NSAM_2048, SMODE
    ioctl(fd, FFT_ACC_SET_CFG, &cfg);
    
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        ioctl(fd, FFT_ACC_LOAD_DATA, &sine_wave[i]);
    }
    
}

void read_results(int fd, uint32_t *results) {
    
    ioctl(fd, FFT_ACC_GET_RESULT, &results);

}

int main() {
    int fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open FFT accelerator device");
        return EXIT_FAILURE;
    }

    uint32_t sine_wave[NUM_SAMPLES];
    uint32_t results[NUM_SAMPLES / 2];

    // Test for 32-bit samples
    printf("Testing FFT Accelerator with 32-bit sine wave samples\n");
    generate_sine_wave(sine_wave, 32);
    load_samples(fd, sine_wave, 32);
    read_results(fd, results);
    
    for (uint32_t i = 0; i < NUM_SAMPLES / 2; i++) {
        printf("FFT Result[%u]: %u\n", i, results[i]);
    }

    // Test for 16-bit samples
    printf("Testing FFT Accelerator with 16-bit sine wave samples\n");
    generate_sine_wave(sine_wave, 16);
    load_samples(fd, sine_wave, 16);
    read_results(fd, results);
    
    for (uint32_t i = 0; i < NUM_SAMPLES / 2; i++) {
        printf("FFT Result[%u]: %u\n", i, results[i]);
    }

    close(fd);
    return EXIT_SUCCESS;
}
