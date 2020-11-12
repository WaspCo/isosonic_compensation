/* @file main.c
 * Isosonic Compensation
 *
 * Given a listening level in dbSPL at which the audio material was supposedly
 * mixed, and a listening level at which the audio material is being listened
 * to, automatically apply a spectrum correction as to compensate the effect
 * of the isosonic curves of the human ear.
 *
 * Auteur : Victor Deleau
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

#include <fftw3.h>

#include "wav.h"
#include "dft.h"
#include "isosonic.h"
#include "loudness.h"
#include "allocation.h"

#define NB_CURVE 90

int main(int argc, char *argv[])
{
    clock_t begin = clock();

    printf("---------------------------------------------------------------\n");
    printf("------------------------------ isosonic compensation ----------\n");
    printf("---------------------------------------------------------------\n");
    printf("\n");

    ////////////////////////////////////////////////////////////////////////////
    // 1) setup ////////////////////////////////////////////////////////////////

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s [input_file]* [output_file]* [buffer_size]* [listening_level]\n", argv[0]);
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");
    if (!input_file)
    {
        fprintf(stderr, "Error while opening the input file. (%s)\n", argv[1]);
        exit(1);
    }

    FILE *output_file = fopen(argv[2], "w");
    if (!output_file)
    {
        fprintf(stderr, "Error while allocating the output file. (%s)\n", argv[2]);
        exit(1);
    }

    Header input_file_header;
    header_wav_read(&input_file_header, input_file);

    // get the specified buffer size

    int b = 0;
    if ((argc > 3 && sscanf(argv[3], "%u", &b) != 1) || (b < 8) || (b > 1048576))
    {
        fprintf(stderr, "Incorrect buffer size (%s) (8 <= buffer_size <= 1048576)\n", argv[3]);
        exit(1);
    }
    const size_t BUFFER_SIZE = b;

    b = 0;
    if ((argc > 3 && sscanf(argv[4], "%u", &b) != 1) || (b <= 0) || (b > 80))
    {
        fprintf(stderr, "Incorrect listening level (%s)\n (0 < listening_level <= 80)\n", argv[4]);
        exit(1);
    }
    const size_t LISTENING_LEVEL = b;

    // read isosonic curves and make transfer functions

    FILE *isosonic_file = NULL;
    isosonic_file = fopen("curve_processed.csv", "r");

    if (!isosonic_file)
    {
        isosonic_file = process_isosonic_curve();

        if (!isosonic_file)
        {
            fprintf(stderr, "Error while processing the isosonic curves.");
            exit(1);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // 2) memory allocation ////////////////////////////////////////////////////

    int64_t *L_input_time = allocate_buffer(BUFFER_SIZE);
    int64_t *R_input_time = allocate_buffer(BUFFER_SIZE);

    float *L_overlap_time = malloc(sizeof(float) * (BUFFER_SIZE));
    float *R_overlap_time = malloc(sizeof(float) * (BUFFER_SIZE));

    const size_t NB_BINS = (BUFFER_SIZE / 2) + 1;

    fftw_complex *R_spectrum = fftw_malloc(sizeof(fftw_complex) * NB_BINS);
    fftw_complex *L_spectrum = fftw_malloc(sizeof(fftw_complex) * NB_BINS);

    int64_t *L_output_time = allocate_buffer(BUFFER_SIZE);
    int64_t *R_output_time = allocate_buffer(BUFFER_SIZE);

    if (L_input_time == NULL || R_input_time == NULL || L_output_time == NULL || R_output_time == NULL || L_overlap_time == NULL || R_overlap_time == NULL || R_spectrum == NULL || L_spectrum == NULL)
    {
        fprintf(stderr, "Error while allocating the buffers.\n");
        exit(1);
    }

    TransferFunction **transfer_functions = NULL;
    transfer_functions = allocate_transfer_function(BUFFER_SIZE * 2, NB_CURVE);

    if (!transfer_functions)
    {
        fprintf(stderr, "Error while allocating the transfer functions.\n");
        exit(1);
    }

    craft_transfer_functions(isosonic_file, transfer_functions, BUFFER_SIZE);

    ////////////////////////////////////////////////////////////////////////////
    // 3) signal processing ////////////////////////////////////////////////////

    // write the header of the wav output file
    header_wav_write(&input_file_header, output_file);

    // display it for info sake
    display_wav_header(&input_file_header);

    // how many iterations of the processing loop ?
    size_t nb_lecture = (input_file_header.nb_block / (BUFFER_SIZE / 2));

    // get the incomplete last buffer if needed
    if (input_file_header.nb_block % (BUFFER_SIZE / 2) > 0)
        nb_lecture += 1;

    // hold on just a second
    char choice[3] = {0};
    printf("Apply compensation ? (y/Y/n/N)\t");
    scanf("%c", choice);
    if (choice[0] == 'n' || choice[0] == 'N')
        exit(1);

    printf("\n------------------------------ Processing has started .....\n\n");

    size_t offset = (BUFFER_SIZE / 2) * input_file_header.block_size;

    size_t cumulative_read = 0;
    size_t total_samples_read = 0;

    size_t curve2apply = get_curve_from_listening_level(
        transfer_functions,
        LISTENING_LEVEL);

    TransferFunction *transfer_function = transfer_functions[curve2apply];

    allocate_dft(BUFFER_SIZE); // required before using dft_forward/dft_backward

    for (size_t i = 0; i < (nb_lecture); i++)
    {
        size_t remaining_samples = (input_file_header.nb_block) - (cumulative_read);

        printf("------------------------------------ window n° %5u", i);

        size_t samples_to_read = fmin(BUFFER_SIZE, remaining_samples);
        size_t block_samples_read;

        block_samples_read = data_wav_read(
            samples_to_read,
            &input_file_header,
            L_input_time,
            R_input_time,
            input_file);

        if (block_samples_read == -1)
        {
            fprintf(stderr, "Error while reading data from input WAV file.\n");
            exit(1);
        }

        printf(" (%d/%d) -----\n", block_samples_read, samples_to_read);
        cumulative_read += (block_samples_read / 2);

        dft_forward(R_input_time, R_spectrum, BUFFER_SIZE, NB_BINS);
        dft_forward(L_input_time, L_spectrum, BUFFER_SIZE, NB_BINS);

        spectrum_product(R_spectrum, transfer_function, NB_BINS);
        spectrum_product(L_spectrum, transfer_function, NB_BINS);

        dft_backward(R_spectrum, R_output_time, BUFFER_SIZE, NB_BINS);
        dft_backward(L_spectrum, L_output_time, BUFFER_SIZE, NB_BINS);

        block_samples_read = data_wav_write(
            samples_to_read,
            &input_file_header,
            L_output_time,
            R_output_time,
            output_file);

        if (block_samples_read == -1)
        {
            fprintf(stderr, "Error while writing data to output WAV file.\n");
            exit(1);
        }

        total_samples_read += (block_samples_read / 2);

        // overlapp-add (rewind pointer of "offset" bytes)
        fseek(input_file, ftell(input_file) - offset, SEEK_SET);
    }

    printf("Alt er vel (^_^)\n");

    update_wav_header(output_file, &input_file_header);

    ////////////////////////////////////////////////////////////////////////////
    // 3) cleanup //////////////////////////////////////////////////////////////

    deallocate_dft();

    deallocate_transfer_functions(transfer_functions, NB_CURVE);

    free(L_input_time);
    free(R_input_time);

    free(L_overlap_time);
    free(R_overlap_time);

    free(L_spectrum);
    free(R_spectrum);

    free(L_output_time);
    free(R_output_time);

    if (fclose(input_file))
    {
        printf("Error while closing the input file.");
        exit(1);
    }

    if (fclose(output_file))
    {
        printf("Error while closing the output file.");
        exit(1);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time: %lf seconds\n", time_spent);

    return 0;
}