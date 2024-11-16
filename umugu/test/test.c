
#include "../src/umugu_internal.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void print_vector(const char *title, um_complex *x, int n) {
    printf("%s (dim=%d):", title, n);
    for (int i = 0; i < n; i++) {
        printf(" %5.2f,%5.2f ", x[i].real, x[i].imag);
    }
    printf("\n");
}

static void test_fft() {
    static const int N = 1 << 3; /* N-point FFT, iFFT */
    um_complex v[N], v1[N], scratch[N];
    int k;

    /* Fill v[] with a function of known FFT: */
    for (k = 0; k < N; k++) {
        v[k].real = 0.125 * cos(2 * M_PI * k / (double)N);
        v[k].imag = 0.125 * sin(2 * M_PI * k / (double)N);
        v1[k].real = 0.3 * cos(2 * M_PI * k / (double)N);
        v1[k].imag = -0.3 * sin(2 * M_PI * k / (double)N);
    }

    /* FFT, iFFT of v[]: */
    print_vector("Orig", v, N);
    um_fft(v, N, scratch);
    print_vector(" FFT", v, N);
    um_ifft(v, N, scratch);
    print_vector("iFFT", v, N);

    /* FFT, iFFT of v1[]: */
    print_vector("Orig", v1, N);
    um_fft(v1, N, scratch);
    print_vector(" FFT", v1, N);
    um_ifft(v1, N, scratch);
    print_vector("iFFT", v1, N);
}

int main(void) {
    test_fft();
    return 0;
}
