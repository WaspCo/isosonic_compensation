/* @file loudness.c
 * Allocation and de-allocation of the heap memory
 *
 * Auteurs:
 * Victor Deleau
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "isosonic.h"
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


/** Allocate a list of transfer_function and initialize them with zeros
 * @param NB_DATA_POINT (size_t const)
 * @param NB_TRANSFER_FUNCTIONS of transfer functions to allocate(size_t const)
 * @return the allocated list of transfer functions (TransferFunction[])
 */
TransferFunction **allocate_transfer_function(
    size_t const NB_DATA_POINT,
    size_t const NB_TRANSFER_FUNCTIONS)
{
    TransferFunction **transfer_functions;

    size_t nb_bytes = sizeof(TransferFunction *) * NB_TRANSFER_FUNCTIONS;

    transfer_functions = (TransferFunction**)malloc(nb_bytes);

    size_t struct_bytes = sizeof(TransferFunction);
    size_t data_bytes = sizeof(float) * NB_DATA_POINT;

    for (size_t i = 0; i < NB_TRANSFER_FUNCTIONS; i++)
    {

        transfer_functions[i] = (TransferFunction*)malloc(struct_bytes);

        transfer_functions[i]->data = (float *)malloc(data_bytes);

        memset(transfer_functions[i]->data, 0, data_bytes);

        transfer_functions[i]->level_at_1000hz = 0;
    }

    return transfer_functions;
}

/** De-allocate a transfer function
 * @param transfer_functions (TransferFunction[])
 * @param NB_TRANSFER_FUNCTIONS of transfer functions in argument (size_t const)
 * @return 0 if ok, 1 otherwise
 */
uint8_t deallocate_transfer_functions(
    TransferFunction **transfer_functions,
    size_t const NB_TRANSFER_FUNCTIONS)
{

    for (size_t i = 0; i < NB_TRANSFER_FUNCTIONS; i++)
    {
        free(transfer_functions[i]->data);

        free(transfer_functions[i]);
    }

    free(transfer_functions);

    if (transfer_functions)
    {
        return 1;
    }

    return 0;
}