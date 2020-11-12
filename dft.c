/* @file dft.c
 * Fast Discrete Fourier Transform
 *
 * Source:
 * - https://en.wikipedia.org/wiki/Discrete_Fourier_transform
 * Auteur:
 * Victor Deleau
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include <fftw3.h>

#include "loudness.h"
#include "isosonic.h"
#include "dft.h"

#define REAL 0
#define IMAG 1

/** Perform a circular shifting of half of the array
 * @param buffer to circshift (double [])
 * @param BUFFER_SIZE (size_t const)
 * @return 0 if ok, 1 otherwise (uint_8)
 */
uint8_t circshift(
    double buffer[],
    size_t const BUFFER_SIZE)
{
    double tmp_buffer[BUFFER_SIZE / 2];

    for (size_t i = 0; i < (BUFFER_SIZE / 2 - 1); i++)
    {
        tmp_buffer[i] = buffer[i];
    }

    for (size_t i = 0; i < (BUFFER_SIZE / 2 - 1); i++)
    {
        buffer[i] = buffer[i + (BUFFER_SIZE / 2)];
        buffer[i + (BUFFER_SIZE / 2)] = tmp_buffer[i];
    }

    return 0;
}

/** Allocate the buffers required for the DFT computation
 * @param BUFFER_SIZE (size_t const)
 * @return 0 if ok, 1 otherwise (uint_8)
 */
uint8_t allocate_dft(size_t const BUFFER_SIZE) {

    input_time_normalized = malloc(sizeof(double) * BUFFER_SIZE);
    output_time_normalized = malloc(sizeof(double) * BUFFER_SIZE);

    if (input_time_normalized == NULL || output_time_normalized == NULL)
    {
        fprintf(stderr, "Error while allocating memory for the dft.\n");
        return 1;
    }

    return 0;
}

/** De-allocate the buffers required for the DFT computation
 * @return 0 if ok, 1 otherwise (uint_8)
 */
uint8_t deallocate_dft() {

    free(input_time_normalized);
    free(output_time_normalized);

    return 0;
}


/** Perform forward FFT on a time buffer
 * @param input_time (int64_t const [])
 * @param output_spectrum (fftw_complex [])
 * @param BUFFER_SIZE (size_t const)
 * @param NB_BINS (size_t const)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t dft_forward(
    int64_t const input_time[],
    fftw_complex output_spectrum[],
    size_t const BUFFER_SIZE,
    size_t const NB_BINS)
{

    if (input_time_normalized == NULL | output_time_normalized == NULL)
    {
        fprintf(stderr, "Error: dft memory is not allocated. Please first run allocate_dft().\n");
        return 1;
    }

    // double pi = 3.14159265359;

    // int64 to double conversion
    for (size_t i = 0; i < BUFFER_SIZE; i++)
    {
        input_time_normalized[i] = (double)input_time[i] / 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));		// test hann window;
    }

    circshift(input_time_normalized, BUFFER_SIZE);

    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(
        NB_BINS,
        input_time_normalized,
        output_spectrum,
        FFTW_MEASURE);

    fftw_execute(plan_forward);

    return 0;
}

/** Perform backward FFT on a spectrum buffer
 * @param input_spectrum (fftw_complex const [])
 * @param output_time (int64_t [])
 * @param BUFFER_SIZE (size_t const)
 * @param NB_BINS (size_t const)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t dft_backward(
    fftw_complex const input_spectrum[],
    int64_t output_time[],
    size_t const BUFFER_SIZE,
    size_t const NB_BINS)
{

    if (input_time_normalized == NULL | output_time_normalized == NULL)
    {
        fprintf(stderr, "Error: dft memory is not allocated. Please first run allocate_dft().\n");
        return 1;
    }

    fftw_plan plan_backward = fftw_plan_dft_c2r_1d(
        NB_BINS,
        (fftw_complex *)input_spectrum, // drop the const ( smart move ;] )
        output_time_normalized,
        FFTW_MEASURE);

    fftw_execute(plan_backward);

    circshift(output_time_normalized, BUFFER_SIZE);

    // double to int64 convertion
    for (size_t i = 0; i < BUFFER_SIZE; i++)
    {
        output_time[i] = output_time_normalized[i] / ((2 * BUFFER_SIZE) - 1) * 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
    }

    return 0;
}

/** Perform an overlap-add between previous time block and new time block
 * @param input_time (float [])
 * @param memory_time (double [])
 * @param BUFFER_SIZE (size_t const)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t overlap_add(
    float input_time[],
    double memory_time[],
    size_t const BUFFER_SIZE)
{
    // Add new P1 to old P2
    for (size_t i = 0; i < (BUFFER_SIZE / 2); i++)
    {
        input_time[i] += memory_time[i];
    }

    // memorise new P2 (will become will be added to next P1)
    size_t j = 0;
    for (size_t i = (BUFFER_SIZE / 2); i < BUFFER_SIZE; i++)
    {
        input_time[j] = memory_time[i];
        j++;
    }

    return 0;
}