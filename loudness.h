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
    float **data;
    float *metadata;
    size_t nb_curve;
    size_t buffer_size;
}; // Les metadatas sont le niveau d'origine du spectre à 1kH.

typedef struct transfer_function TransferFunction;

uint8_t loudness(
    const size_t BUFFER_SIZE,
    fftw_complex left_input_spectrum[],
    fftw_complex right_input_spectrum[],
    fftw_complex left_output_spectrum[],
    fftw_complex right_output_spectrum[],
    TransferFunction *transfer_function,
    const size_t LISTENING_LEVEL);

#endif // LOUDNESS_H 1