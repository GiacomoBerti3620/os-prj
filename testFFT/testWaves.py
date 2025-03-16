import numpy as np
import matplotlib.pyplot as plt

# Constants
sampling_rate = 10000  # Hz (samples per second)
duration = 0.1024      # seconds
freq1 = 300            # Hz
freq2 = 550            # Hz
amplitude = 127.5     # Amplitude to scale the waveform between 0 and 255
offset = 127.5        # Offset to center waveform around 127.5 (half of 255)
min_value = 0         # Minimum value (0)
max_value = 255       # Maximum value (255)

# Generate time values
t = np.linspace(0, duration, int(sampling_rate * duration), endpoint=False)

# Generate sine waves
wave1 = np.sin(2 * np.pi * freq1 * t)
wave2 = np.sin(2 * np.pi * freq2 * t)

# Compose the waves
composed_wave = wave1 + wave2

# Scale to range [0, 255] without clipping
# Find the min and max values in the composed wave
composed_wave_min = np.min(composed_wave)
composed_wave_max = np.max(composed_wave)
 
# Scale the composed wave to [0, 255] range
scaled_wave = (composed_wave - composed_wave_min) / (composed_wave_max - composed_wave_min)
scaled_wave = scaled_wave * (max_value - min_value) + min_value

# Convert to integer values
samples = np.round(scaled_wave).astype(int)

# Dump as C integer vector
def dump_as_c_vector(samples):
    print("static int wave_samples[] = {")
    for i, sample in enumerate(samples):
        if i != len(samples) - 1:
            print(f"{sample}, ", end="")
        else:
            print(f"{sample}", end="")
    print("\n};")

# Function to read the integer array from a file
def read_samples_from_file(filename):
    with open(filename, 'r') as file:
        content = file.read()
        # Assuming the array is a standard C array and formatted correctly
        start = content.find("{") + 1
        end = content.find("}")
        array_string = content[start:end]
        # Convert the comma-separated values to a list of integers
        samples = list(map(int, array_string.split(',')))
        return samples

# Plot the original and composed waves
def plot_waveforms():
    plt.figure(figsize=(12, 6))
    
    # Plot original sine waves
    plt.subplot(2, 1, 1)
    plt.plot(t, wave1, label="Sine Wave 1", color="blue")
    plt.plot(t, wave2, label="Sine Wave 2", color="orange")
    plt.title("Original Sine Waves")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")
    plt.legend()
    
    # Plot composed wave (normalized)
    plt.subplot(2, 1, 2)
    plt.plot(t, scaled_wave, label="Composed Wave", color="green")
    plt.title("Composed Wave")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude (0 to 255)")
    plt.legend()

    plt.tight_layout()
    plt.show()

# Function to plot the integer samples read from a file
def plot_samples_from_file(filename):
    samples = read_samples_from_file(filename)

    # Compute the corresponding frequencies
    n = len(samples)
    freqs = np.fft.fftfreq(n, d=1/sampling_rate)

    # Resize to be half of the initial wave (only positive frequencies)
    positive_samples = samples[:n//2]
    positive_freqs = freqs[:n//2]

    # Discard DC component
    positive_samples = positive_samples[1:]
    positive_freqs = positive_freqs[1:]

    plt.figure(figsize=(10, 5))
    plt.plot(positive_freqs, positive_samples, label="Integer Samples from File", color="red")
    plt.title(f"Integer Samples from {filename}")
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Amplitude (0 to 255)")
    plt.legend()
    plt.show()

# Uncomment this to generate the wave and dump it as C vector
dump_as_c_vector(samples)

# Uncomment this to plot the original and composed waveforms
plot_waveforms()

# To read and plot the samples from a file
plot_samples_from_file("postFFT.txt")  # Replace with the actual path to your C file

