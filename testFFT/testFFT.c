#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#define SAMPLES_COUNT_MAX 2048

// Sample data
static int wave_samples[] = {
128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132, 123, 109, 92, 74, 58, 45, 38, 39, 48, 65, 89, 118, 150, 182, 211, 234, 249, 255, 251, 238, 216, 189, 159, 129, 102, 79, 63, 55, 55, 61, 72, 86, 100, 114, 123, 128, 128, 122, 112, 101, 89, 79, 73, 73, 80, 93, 113, 138, 166, 193, 218, 237, 248, 251, 244, 227, 203, 173, 139, 106, 76, 51, 33, 24, 24, 32, 47, 66, 87, 108, 126, 139, 147, 148, 145, 137, 127, 118, 110, 107, 108, 116, 129, 147, 168, 189, 208, 223, 231, 231, 222, 204, 179, 149, 116, 82, 52, 28, 11, 4, 7, 18, 37, 62, 89, 117, 142, 162, 175, 182, 182, 176, 166, 154, 143, 133, 127, 127, 132, 141, 155, 169, 183, 194, 200, 200, 192, 176, 153, 126, 96, 66, 39, 17, 4, 0, 6, 21, 44, 73, 105, 137, 166, 190, 207, 216, 217, 210, 197, 181, 163, 146, 132, 123, 120, 121, 128, 137, 147, 157, 163, 165, 160, 149, 132, 111, 86, 62, 40, 23, 13, 12, 21, 38, 63, 94, 128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132, 123, 109, 92, 74, 58, 45, 38, 39, 48, 65, 89, 118, 150, 182, 211, 234, 249, 255, 251, 238, 216, 189, 159, 129, 102, 79, 63, 55, 55, 61, 72, 86, 100, 114, 123, 128, 127, 122, 112, 101, 89, 79, 73, 73, 80, 93, 113, 138, 166, 193, 218, 237, 248, 251, 244, 227, 203, 173, 139, 106, 76, 51, 33, 24, 24, 32, 47, 66, 87, 108, 126, 139, 147, 148, 145, 137, 127, 118, 110, 107, 108, 116, 129, 147, 168, 189, 208, 223, 231, 231, 222, 204, 179, 149, 116, 82, 52, 28, 11, 4, 7, 18, 37, 62, 89, 117, 142, 162, 175, 182, 182, 176, 166, 154, 143, 133, 127, 127, 132, 141, 155, 169, 183, 194, 200, 200, 192, 176, 153, 126, 96, 66, 39, 17, 4, 0, 6, 21, 44, 73, 105, 137, 166, 190, 207, 216, 217, 210, 197, 181, 163, 146, 132, 123, 120, 121, 128, 137, 147, 157, 163, 165, 160, 149, 132, 111, 86, 62, 40, 23, 13, 12, 21, 38, 63, 94, 128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132, 123, 109, 92, 74, 58, 45, 38, 39, 48, 65, 89, 118, 150, 182, 211, 234, 249, 255, 251, 238, 216, 189, 159, 129, 102, 79, 63, 55, 55, 61, 72, 86, 100, 114, 123, 128, 128, 122, 112, 101, 89, 79, 73, 73, 80, 93, 113, 138, 166, 193, 218, 237, 248, 251, 244, 227, 203, 173, 139, 106, 76, 51, 33, 24, 24, 32, 47, 66, 87, 108, 126, 139, 147, 148, 145, 137, 127, 118, 110, 107, 108, 116, 129, 147, 168, 189, 208, 223, 231, 231, 222, 204, 179, 149, 116, 82, 52, 28, 11, 4, 7, 18, 37, 62, 89, 117, 142, 162, 175, 182, 182, 176, 166, 154, 143, 133, 127, 127, 132, 141, 155, 169, 183, 194, 200, 200, 192, 176, 153, 126, 96, 66, 39, 17, 4, 0, 6, 21, 44, 73, 105, 137, 166, 190, 207, 216, 217, 210, 197, 181, 163, 146, 132, 123, 120, 121, 128, 137, 147, 157, 163, 165, 160, 149, 132, 111, 86, 62, 40, 23, 13, 12, 21, 38, 63, 94, 128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132, 123, 109, 92, 74, 58, 45, 38, 39, 48, 65, 89, 118, 150, 182, 211, 234, 249, 255, 251, 238, 216, 189, 159, 129, 102, 79, 63, 55, 55, 61, 72, 86, 100, 114, 123, 128, 128, 122, 112, 101, 89, 79, 73, 73, 80, 93, 113, 138, 166, 193, 218, 237, 248, 251, 244, 227, 203, 173, 139, 106, 76, 51, 33, 24, 24, 32, 47, 66, 87, 108, 126, 139, 147, 148, 145, 137, 127, 118, 110, 107, 108, 116, 129, 147, 168, 189, 208, 223, 231, 231, 222, 204, 179, 149, 116, 82, 52, 28, 11, 4, 7, 18, 37, 62, 89, 117, 142, 162, 175, 182, 182, 176, 166, 154, 143, 133, 127, 127, 132, 141, 155, 169, 183, 194, 200, 200, 192, 176, 153, 126, 96, 66, 39, 17, 4, 0, 6, 21, 44, 73, 105, 137, 166, 190, 207, 216, 217, 210, 197, 181, 163, 146, 132, 123, 120, 121, 128, 137, 147, 157, 163, 165, 160, 149, 132, 111, 86, 62, 40, 23, 13, 12, 21, 38, 63, 94, 128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132, 123, 109, 92, 74, 58, 45, 38, 39, 48, 65, 89, 118, 150, 182, 211, 234, 249, 255, 251, 238, 216, 189, 159, 129, 102, 79, 63, 55, 55, 61, 72, 86, 100, 114, 123, 128, 127, 122, 112, 101, 89, 79, 73, 73, 80, 93, 113, 138, 166, 193, 218, 237, 248, 251, 244, 227, 203, 173, 139, 106, 76, 51, 33, 24, 24, 32, 47, 66, 87, 108, 126, 139, 147, 148, 145, 137, 128, 118, 110, 107, 108, 116, 129, 147, 168, 189, 208, 223, 231, 231, 222, 204, 179, 149, 116, 82, 52, 28, 11, 4, 7, 18, 37, 62, 89, 117, 142, 162, 175, 182, 182, 176, 166, 154, 143, 133, 128, 127, 132, 141, 155, 169, 183, 194, 200, 200, 192, 176, 153, 126, 96, 66, 39, 17, 4, 0, 6, 21, 44, 73, 105, 137, 166, 190, 207, 216, 217, 210, 197, 181, 163, 146, 132, 123, 120, 121, 128, 137, 147, 157, 163, 165, 160, 149, 132, 111, 86, 62, 40, 23, 13, 12, 21, 38, 63, 94, 128, 161, 192, 217, 234, 243, 242, 232, 215, 193, 169, 144, 123, 106, 95, 90, 92, 98, 108, 118, 128, 134, 135, 132
};

// Function to reorder the complex array based on the reverse index
// Only real part is reordered since imaginary is just zeros
void sort(int *f_real, int N) {
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
void FFT(int *f_real, int *f_imag, int N) {
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

// Function to print the array to a file
void print_array_to_file(int *array, int length, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Unable to open file for writing");
        return;
    }

    fprintf(file, "static int wave_samples[] = {\n");

    for (int i = 0; i < length; ++i) {
        if (i != length - 1) {
            fprintf(file, "%d, ", array[i]);
        } else {
            fprintf(file, "%d", array[i]);
        }
    }

    fprintf(file, "\n};\n");

    fclose(file);
}

int main(int argc, char **argv) {
    int wave_samples_real[SAMPLES_COUNT_MAX];
    int wave_samples_imag[SAMPLES_COUNT_MAX];

    // EMULATED CFG0 REGISTER
    int samplesCount = 1024;
    int smodeMaxVal = 255;

    // Copy the real values from the original sample
    for (int i = 0; i < samplesCount; i++) {
        wave_samples_real[i] = wave_samples[i];
        wave_samples_imag[i] = 0;  // Imaginary part is 0 initially
    }

    // TEST FFT Alogithm
    FFT(wave_samples_real, wave_samples_imag, samplesCount);

    // Compute magnitudes
    for (int i = 0; i < samplesCount; i++)
        wave_samples[i] = sqrt(pow(wave_samples_real[i], 2) + pow(wave_samples_imag[i], 2));
    
    // Find max and min
    int min_val = wave_samples[0];
    int max_val = 0;

    for (int i = 1; i < samplesCount; i++) {
        if (wave_samples[i] < min_val) {
            min_val = wave_samples[i];
        }
        if (wave_samples[i] > max_val) {
            max_val = wave_samples[i];
        }
    }

    // Scale all samples
    for (int i = 0; i < samplesCount; i++)
    {
        if (max_val == min_val)
            wave_samples[i] = smodeMaxVal/2;
        else
            wave_samples[i] = (float)(wave_samples[i] - min_val) / (max_val - min_val) * (float)(smodeMaxVal);
    }
    
    // Save the result to a file
    print_array_to_file(wave_samples, samplesCount, "postFFT.txt");

    return 0;
}