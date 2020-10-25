#ifndef LOUDNESS_H
#define LOUDNESS_H 1

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <fftw3.h>

#include "wav.h"

struct transfer_function
{ // Contient le spectre de la réponse impulsionelle
    double **data;
    double metadata[90];
}; // Les metadatas sont le niveau d'origine du spectre à 1kH.

int loudness(
    Header *header,
    unsigned int *buffer_size,
    fftw_complex dft_freq_L[],
    fftw_complex dft_freq_R[],
    fftw_complex dft_freq_conv_L[],
    fftw_complex dft_freq_conv_R[],
    struct transfer_function *curve_processed,
    int level);

// float get_db(Header *header, unsigned int *buffer_size, int64_t left[], int64_t right[]);

#endif // LOUDNESS_H 1
