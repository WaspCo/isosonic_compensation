#ifndef ALLOCATION_H
#define ALLOCATION_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "loudness.h"

int64_t *allocate_buffer(size_t size);

TransferFunction **allocate_transfer_function(
    size_t const NB_DATA_POINT,
    size_t const NB_TRANSFER_FUNCTIONS);

uint8_t deallocate_transfer_functions(
    TransferFunction **transfer_functions,
    size_t const NB_TRANSFER_FUNCTIONS);

#endif // ALLOCATION_H 1