#ifndef DFT_H
#define DFT_H 1

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

static double *input_time_normalized = NULL;
static double *output_time_normalized = NULL;

uint8_t allocate_dft(size_t const BUFFER_SIZE);
uint8_t deallocate_dft();

uint8_t circshift(
    double buffer[],
    size_t const BUFFER_SIZE);

uint8_t dft_forward(
    int64_t const input_time[],
    fftw_complex output_spectrum[],
    size_t const BUFFER_SIZE,
    size_t const NB_BINS);

uint8_t dft_backward(
    fftw_complex const input_spectrum[],
    int64_t output_time[],
    size_t const BUFFER_SIZE,
    size_t const NB_BINS);

uint8_t overlap_add(
    float input_time[],
    double memory_time[],
    size_t const BUFFER_SIZE);

#endif // DFT_H 1