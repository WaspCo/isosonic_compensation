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
#include "fft.h"
#include "isosonic.h"
#include "loudness.h"
#include "allocation.h"

#define NB_CURVE 90

int main(int argc, char *argv[])
{
    clock_t begin = clock();

    printf("---------------------------------------------------------------------\n");
    printf("------------------------------------ isosonic compensation ----------\n");
    printf("---------------------------------------------------------------------\n\n");

    ////////////////////////////////////////////////////////////////////////////
    // 1) I/O //////////////////////////////////////////////////////////////////

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s [input_file]* [output_file]* [buffer_size]* [listening_level]\n", argv[0]);
        exit(1);
    }

    FILE *input = fopen(argv[1], "r");
    if (!input)
    {
        fprintf(stderr, "Error while opening the input file. (%s)\n", argv[1]);
        exit(1);
    }

    FILE *output = fopen(argv[2], "w");
    if (!output)
    {
        fprintf(stderr, "Error while allocating the output file. (%s)\n", argv[2]);
        exit(1);
    }

    // read header of input .wav file
    Header header;
    header_read(&header, input);

    
    // get the specified buffer size
    int b = 0;
    if ((argc>3 && sscanf(argv[3], "%u", &b) != 1) || (b < 8) || (b > 1048576))
    {
        fprintf(stderr, "Incorrect buffer size (%s) (8 <= buffer_size <= 1048576)\n", argv[3]);
        exit(1);
    }
    const size_t BUFFER_SIZE = b;

    // get the listening level
    b = 0;
    ;
    if ((argc>3 && sscanf(argv[4], "%u", &b) != 1) || (b <= 0) || (b > 80))
    {
        fprintf(stderr, "Incorrect listening level (%s)\n (0 < listening_level <= 80)\n", argv[4]);
        exit(1);
    }
    const size_t LISTENING_LEVEL = b;

    // allocate isosonic curves on the heap and load

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

    TransferFunction *transfer_function;

    transfer_function = allocate_transfer_function(BUFFER_SIZE, NB_CURVE);

    if (!transfer_function) exit(1);

    craft_transfer_function(isosonic_file, transfer_function, BUFFER_SIZE);

    ////////////////////////////////////////////////////////////////////////////
    // 2) working buffers allocation ///////////////////////////////////////////

    int64_t *left_input_buffer = allocate_buffer(BUFFER_SIZE);
    int64_t *right_input_buffer = allocate_buffer(BUFFER_SIZE);

    int64_t *left_output_buffer = allocate_buffer(BUFFER_SIZE);
    int64_t *right_output_buffer = allocate_buffer(BUFFER_SIZE);

    // temp storage for P2
    float *left_overlap_buffer = malloc(sizeof(float) * (BUFFER_SIZE));
    float *right_overlap_buffer = malloc(sizeof(float) * (BUFFER_SIZE));

    if (left_input_buffer == NULL || right_input_buffer == NULL || left_output_buffer == NULL || right_output_buffer == NULL)
    {
        fprintf(stderr, "Error while allocating the buffers.\n");
        exit(1);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 3) parsing / processing /////////////////////////////////////////////////

    // write the header of the wav output file
    header_write(&header, output);

    // display it for info sake
    display_header(&header);

    // how many iterations of the processing loop ?
    unsigned int nb_lecture = (header.nb_block / (BUFFER_SIZE / 2));

    // get the incomplete last buffer if needed
    if (header.nb_block % (BUFFER_SIZE / 2) > 0)
        nb_lecture += 1;

    // hold on just a second
    char choice[3] = {0};
    printf("Go ? (y/Y/n/N)\t");
    scanf("%c", choice);
    if (choice[0] == 'n' || choice[0] == 'N')
        exit(1);

    printf("\n------------------------------------ Processing has started .....\n\n");

    unsigned int offset = (BUFFER_SIZE / 2) * header.block_size;

    size_t cumulative_read = 0, cumulative_write = 0;

    // main processing loop
    for (unsigned int i = 0; i < (nb_lecture); i++)
    {
        size_t remaining = (header.nb_block) - (cumulative_read);

        printf("------------------------------------ window n° %5u", i);

        size_t bytes_to_read = fmin(BUFFER_SIZE, remaining);
        size_t count;

        // read data from disk
        count = data_read(bytes_to_read, &header, left_input_buffer, right_input_buffer, input);

        if (count == -1)
        {
            fprintf(stderr, "Error while reading data from input WAV file.\n");
            exit(1);
        }

        printf(" (%zu/%zu) -----\n", count, bytes_to_read);
        cumulative_read += (count / 2);

        // apply fft + correction
        fft(BUFFER_SIZE, left_input_buffer, right_input_buffer, left_output_buffer, right_output_buffer, left_overlap_buffer, right_overlap_buffer, transfer_function, LISTENING_LEVEL);

        // write back P1 to disk
        count = data_write(bytes_to_read, &header, left_output_buffer, right_output_buffer, output);

        if (count == -1)
        {
            fprintf(stderr, "Error while writing data to output WAV file.\n");
            exit(1);
        }

        cumulative_write += (count / 2);

        // overlapp-add of P2 (effectively rewind pointer of "offset" bytes)
        fseek(input, ftell(input) - offset, SEEK_SET);
    }

    printf("Alt er vel (^_^)\n");
    printf("Sample read: %zu\n", cumulative_read);
    printf("Sample writen: %zu\n", cumulative_write);

    // Update the output file header with up-to-date information about the file

    // chunk size update (nb file bytes - 8)
    unsigned char buffer[4];
    fseek(output, 0L, SEEK_END);
    int new_chunk_size = ftell(output) - 8;
    unsigned_to_buffer(new_chunk_size, buffer, 4);
    fseek(output, 4, SEEK_SET);
    fwrite(buffer, 4, 1, output);

    // sub chunk size update (nb bytes in data part)
    int new_sub_chunk_size2 = new_chunk_size - 36 - (header.info_len) - (header.extra_param_len);
    unsigned_to_buffer(new_sub_chunk_size2, buffer, 4);
    fseek(output, 40 + (header.info_len) + (header.extra_param_len), SEEK_SET);
    fwrite(buffer, 4, 1, output);

    // free the memory !

    deallocate_transfer_function(transfer_function);

    free(left_input_buffer);
    free(right_input_buffer);

    free(left_overlap_buffer);
    free(right_overlap_buffer);

    free(left_output_buffer);
    free(right_output_buffer);

    // close files

    if (fclose(input))
    {
        printf("Error while closing the input file.");
        exit(1);
    }

    if (fclose(output))
    {
        printf("Error while closing the output file.");
        exit(1);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time: %lf seconds\n", time_spent);

    return 0;
}