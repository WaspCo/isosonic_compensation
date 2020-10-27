
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "allocation.h"
#include "loudness.h"

/** Allocate a memory buffer and initialize it with zeros
 * @return a pointer to the allocated memory (int64_t*)
 */
int64_t *allocate_buffer(size_t size)
{
    int64_t *buffer = (int64_t *)malloc(size * sizeof(int64_t));

    if (!buffer)
        return NULL;

    memset(buffer, 0, size * sizeof(int64_t));

    return buffer;
}

/** Allocate a transfer_function struct and initialize it with zeros
 * @param buffer_size (size_t)
 * @param n number of transfer function contained (size_t)
 * @return the allocated transfer function (TransferFunction*)
 */
TransferFunction *allocate_transfer_function(
    size_t buffer_size,
    size_t nb_curve)
{
    TransferFunction *transfer_function;

    transfer_function = (TransferFunction*)malloc(sizeof(TransferFunction));

    size_t nb_bytes = (2 * buffer_size) * sizeof(float *);

    transfer_function->data = (float **)malloc(nb_bytes);

    nb_bytes = nb_curve * sizeof(float);

    for (int i = 0; i < (2 * buffer_size); i++)
    {
        transfer_function->data[i] = (float *)malloc(nb_bytes);
        memset(transfer_function->data[i], 0, nb_bytes);
    }

    transfer_function->metadata = (float *)malloc(nb_bytes);
    memset(transfer_function->metadata, 0, nb_bytes);

    transfer_function->buffer_size = buffer_size;

    transfer_function->nb_curve = nb_curve;

    return transfer_function;
}

/** De-allocate a transfer function
 * @param transfer_function (TransferFunction*)
 * @return 0 if ok, 1 otherwise
 */
uint8_t deallocate_transfer_function(TransferFunction *transfer_function) {

    for (int i = 0; i < (2 * transfer_function->buffer_size); i++)
    {
        free(transfer_function->data[i]);
    }

    free(transfer_function->metadata);

    free(transfer_function);

    if (transfer_function) {
        return 1;
    }

    return 0;
}