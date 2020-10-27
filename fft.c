/* @file fft.c
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

#include "fft.h"
#include "loudness.h"
#include "isosonic.h"

#define REAL 0
#define IMAG 1

/** Perform a circular shifting of half of the array
 * @param left_buffer channel sample block (double[])
 * @param right_buffer channel sample block (double[])
 * @param buffer_size (unsigned int)
 * @return 0 if ok, 1 otherwise (uint_8)
 */
uint8_t circshift(
    double left_buffer[],
    double right_buffer[],
    unsigned int *buffer_size)
{
    double tmp_left[*buffer_size / 2];
    double tmp_right[*buffer_size / 2];

    int i = 0;

    for (i = 0; i < (*buffer_size / 2 - 1); i++)
    {
        tmp_left[i] = left_buffer[i];
        tmp_right[i] = right_buffer[i];
    }

    for (i = 0; i < (*buffer_size / 2 - 1); i++)
    {
        left_buffer[i] = left_buffer[i + (*buffer_size / 2)];
        right_buffer[i] = right_buffer[i + (*buffer_size / 2)];
        left_buffer[i + (*buffer_size / 2)] = tmp_left[i];
        right_buffer[i + (*buffer_size / 2)] = tmp_right[i];
    }

    return 0;
}

/** Perform a FFT on an array of samples
 * @param buffer_size (unsigned int)
 * @param left_input_buffer (int64_t[])
 * @param right_input_buffer (int64_t[])
 * @param left_output_buffer (int64_t[])
 * @param right_output_buffer (int64_t[])
 * @param left_dft_memory (float*)
 * @param right_dft_memory (float*)
 * @param transfer_function to apply (struct transfer_function)
 * @param listening_level (unsigned int)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t fft(
    unsigned int *buffer_size,
    int64_t left_input_buffer[],
    int64_t right_input_buffer[],
    int64_t left_output_buffer[],
    int64_t right_output_buffer[],
    float *left_dft_memory,
    float *right_dft_memory,
    struct transfer_function *transfer_function,
    unsigned int listening_level)
{

    // Variable Declaration
    int n = *buffer_size;
    int fft_length = (n / 2) + 1;
    int i;
    // double pi = 3.14159265359;

    // time audio samples INPUT
    double *dft_time_L;
    double *dft_time_R;

    // freq audio samples FFT OUTPUT
    fftw_complex *dft_freq_L;
    fftw_complex *dft_freq_R;

    // freq audio samples PRODUCT RESULT
    fftw_complex *dft_freq_conv_L;
    fftw_complex *dft_freq_conv_R;

    // memory allocation on the heap

    dft_time_L = malloc(sizeof(double) * n);
    dft_time_R = malloc(sizeof(double) * n);

    dft_freq_L = fftw_malloc(sizeof(fftw_complex) * fft_length);
    dft_freq_R = fftw_malloc(sizeof(fftw_complex) * fft_length);

    dft_freq_conv_L = fftw_malloc(sizeof(fftw_complex) * fft_length);
    dft_freq_conv_R = fftw_malloc(sizeof(fftw_complex) * fft_length);

    // data in & buffers filling
    for (i = 0; i < n; i++)
    { // P1 filling
        dft_time_L[i] = (double)left_input_buffer[i] / 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));		// test hann window;
        dft_time_R[i] = (double)right_input_buffer[i] / 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
    }

    circshift(dft_time_L, dft_time_R, buffer_size);

    fftw_plan left_plan_forward;
    fftw_plan right_plan_forward;

    fftw_plan left_plan_backward;
    fftw_plan right_plan_backward;

    // plan forward fft and apply to left channel
    left_plan_forward = fftw_plan_dft_r2c_1d(
        fft_length, dft_time_L, dft_freq_L, FFTW_MEASURE);
    fftw_execute(left_plan_forward);

    // plan forward fft and apply to right channel
    right_plan_forward = fftw_plan_dft_r2c_1d(
        fft_length, dft_time_R, dft_freq_R, FFTW_MEASURE);
    fftw_execute(right_plan_forward);

    // apply compensation
    loudness(buffer_size, dft_freq_L, dft_freq_R, dft_freq_conv_L, dft_freq_conv_R, transfer_function, listening_level);

    // plan backward fft and apply to left channel
    left_plan_backward = fftw_plan_dft_c2r_1d(
        fft_length, dft_freq_conv_L, dft_time_L, FFTW_MEASURE);
    fftw_execute(left_plan_backward);

    // plan backward fft and apply to right channel
    right_plan_backward = fftw_plan_dft_c2r_1d(
        fft_length, dft_freq_conv_R, dft_time_R, FFTW_MEASURE);
    fftw_execute(right_plan_backward);

    circshift(dft_time_L, dft_time_R, buffer_size);

    // convert samples back to integers
    for (i = 0; i < n; i++)
    {
        dft_time_L[i] = dft_time_L[i] / ((2 * n) - 1) * 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
        dft_time_R[i] = dft_time_R[i] / ((2 * n) - 1) * 0x8000;
        // * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
    }

    for (i = 0; i < (n / 2); i++)
    { // Add new P1 to old P2
        left_output_buffer[i] = left_dft_memory[i] + dft_time_L[i];
        right_output_buffer[i] = right_dft_memory[i] + dft_time_R[i];
    }

    // overlapp-add step
    int j = 0;
    for (int i = (n / 2); i < n; i++)
    {
        left_dft_memory[j] = dft_time_L[i];
        right_dft_memory[j] = dft_time_R[i];
        j++;
    }

    // free the memory

    fftw_destroy_plan(left_plan_forward);
    fftw_destroy_plan(right_plan_forward);

    fftw_destroy_plan(left_plan_backward);
    fftw_destroy_plan(right_plan_backward);

    fftw_free(dft_time_L);
    fftw_free(dft_time_R);

    fftw_free(dft_freq_L);
    fftw_free(dft_freq_R);

    fftw_free(dft_freq_conv_L);
    fftw_free(dft_freq_conv_R);

    return 0;
}
