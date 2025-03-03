#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <iostream>

class FFT {
public:
    using Complex = std::complex<float>;

    // Function to compute FFT, input is real-valued data
    static std::vector<Complex> compute(const std::vector<float>& input) {
        printf("Beginning FFT\n");

        int n = input.size();
        if (n <= 1) return {}; // Early return for small inputs

        // Pad input to the next power of two if needed
        std::vector<float> paddedInput = padToPowerOfTwo(input);  
        std::vector<Complex> data = realToComplex(paddedInput);  // Convert real input to complex
        printf("Input size (after padding): %zu\n", paddedInput.size());

        // Bit-reversal permutation (Rearrange input in bit-reversed order)
        bitReverseReorder(data);

        printf("Past bit reversal reorder\n");

        // Iterative Cooley-Tukey FFT
        for (int s = 0; s < log2(n); s++) {  // Change to < log2(n)
            int m = 1 << s;  // 2^s, size of sub-FFT
            Complex wm = std::polar(1.0f, -2.0f * 3.14159265358979323846f / m); // Twiddle factor
            printf("s = %d, m = %d, wm = (%f, %f)\n", s, m, wm.real(), wm.imag());

            for (int k = 0; k < n; k += m) {
                Complex w = 1;
                for (int j = 0; j < m / 2; j++) {
                    Complex t = w * data[k + j + m / 2];
                    Complex u = data[k + j];
                    data[k + j] = u + t;
                    data[k + j + m / 2] = u - t;
                    w *= wm;
                }
            }
        }

        printf("Complete computation\n");
        return data;  // FFT result in frequency domain
    }

private:
    // Convert real input (float) to complex (with imaginary part = 0)
    static std::vector<Complex> realToComplex(const std::vector<float>& input) {
        std::vector<Complex> complexInput(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            complexInput[i] = Complex(input[i], 0.0f); // Real part = sample, Imaginary = 0
        }
        return complexInput;
    }

    // Bit-reversal permutation function
    static void bitReverseReorder(std::vector<Complex>& data) {
        int n = data.size();
        int j = 0;
        for (int i = 1; i < n; i++) {
            int bit = n >> 1;
            while (j >= bit) {
                j -= bit;
                bit >>= 1;
            }
            j += bit;
            if (i < j) std::swap(data[i], data[j]);
        }
    }

    // Function to get the next power of two
    static int nextPowerOfTwo(int n) {
        return std::pow(2, std::ceil(std::log2(n)));
    }

    // Function to pad the input data
    static std::vector<float> padToPowerOfTwo(const std::vector<float>& input) {
        int n = input.size();
        int nextPow2 = nextPowerOfTwo(n);

        std::vector<float> paddedInput(input);
        paddedInput.resize(nextPow2, 0.0f);  // Resize and pad with zeros

        std::cout << "Padding data to size: " << nextPow2 << std::endl;
        return paddedInput;
    }
};
