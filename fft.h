#ifndef _FFT_H_
#define _FFT_H_ 1

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <fftw3.h>

#include "loudness.h"
#include "isosonic.h"

double circshift(
    double buffer_L[],
    double buffer_R[],
    unsigned int *size);

double fft(
    Header *header,
    unsigned int *buffer_size,
    int64_t input_1_L[],
    int64_t input_1_R[],
    int64_t output_L[],
    int64_t output_R[],
    double *dft_mem_L,
    double *dft_mem_R,
    struct transfer_function *curve_processed,
    int level);

#endif // _FFT_H_ 1
