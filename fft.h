#ifndef FFT_H
#define FFT_H 1

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

uint8_t circshift(
    double left_buffer[],
    double right_buffer[],
    size_t buffer_size);

uint8_t fft(
    size_t buffer_size,
    int64_t left_input_buffer[],
    int64_t right_input_buffer[],
    int64_t left_output_buffer[],
    int64_t right_output_buffer[],
    float *left_overlap_buffer,
    float *right_overlap_buffer,
    TransferFunction *transfer_function,
    unsigned int listening_level);

#endif // FFT_H 1