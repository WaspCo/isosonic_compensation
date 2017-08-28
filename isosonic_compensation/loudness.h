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

struct data {    // Isophonic Curves data structure
  float **c; // The curves ...
  float mtdt[90];  // The metadata
};
struct data c_temp;
struct data c_matrice;

float linear(float *in, float *out, double *precision);

int load_isophonic(FILE* iso_file, struct data *c_matrice, struct data *c_temp, unsigned int *buffer_size);

float get_db(Header *header, unsigned int *buffer_size, int64_t left[], int64_t right[]);

int loudness(Header *header, unsigned int *buffer_size, fftw_complex dft_freq_L[], fftw_complex dft_freq_R[],
    fftw_complex dft_freq_conv_L[], fftw_complex dft_freq_conv_R[], struct data *c_matrice, int level);


#endif // _LOUDNESS_H_ 1
