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
    float *data;
    float level_at_1000hz;
};

typedef struct transfer_function TransferFunction;

uint8_t spectrum_product(
    fftw_complex spectrum[],
    const TransferFunction *transfer_function,
    const size_t NB_BINS);

size_t get_curve_from_listening_level(
    TransferFunction **transfer_functions,
    size_t const LISTENING_LEVEL);

#endif // LOUDNESS_H 1