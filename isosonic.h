#ifndef ISOSONIC_H
#define ISOSONIC_H 1

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "loudness.h"

struct curve
{ // Contient les donnés isosonique + metadata
    float data[90][31];
    float metadata[90];
}; // Les metadatas sont le niveau d'origine des courbes à 1kH.

typedef struct isosonic_curves IsosonicCurves;

struct point
{
    double x;
    double y;
};

double catmul_rom(
    struct point p1,
    struct point p2,
    struct point p3,
    struct point p4,
    double t);

float linear(
    float x,
    float y,
    double t);

uint8_t craft_transfer_functions(
    FILE *isosonic_file,
    TransferFunction **transfer_functions,
    const size_t BUFFER_SIZE);

double catmul_rom(
    struct point p1,
    struct point p2,
    struct point p3,
    struct point p4,
    double t);

FILE *process_isosonic_curve(void);

#endif // ISOSONIC_H 1