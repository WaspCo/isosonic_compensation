#ifndef ALLOCATION_H
#define ALLOCATION_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "loudness.h"

int64_t *allocate_buffer(size_t size);

TransferFunction *allocate_transfer_function(
    size_t buffer_size,
    size_t n);

uint8_t deallocate_transfer_function(TransferFunction *transfer_function);

#endif // ALLOCATION_H 1