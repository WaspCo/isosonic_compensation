#ifndef _LOUDNESS_H_
#define _LOUDNESS_H_ 1

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <fftw3.h>

#include "wave.h"

struct data
{                       // isophonic Curves data structure
    float **data;       // the curve data
    float metadata[90]; // curve metadata
};

struct data curve_raw;
struct data curve_processed;

float linear(
    float *in,
    float *out,
    double *precision);

int load_isophonic(
    FILE *iso_file,
    struct data *curve_raw,
    struct data *curve_processed,
    unsigned int *buffer_size);

int loudness(
    Header *header,
    unsigned int *buffer_size,
    fftw_complex dft_freq_L[],
    fftw_complex dft_freq_R[],
    fftw_complex dft_freq_conv_L[],
    fftw_complex dft_freq_conv_R[],
    struct data *curve_processed,
    int level);

// float get_db(Header *header, unsigned int *buffer_size, int64_t left[], int64_t right[]);

#endif // _LOUDNESS_H_ 1
