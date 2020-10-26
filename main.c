/*
 * Compensation de courbes isophoniques
 * Application principale
 *
 * Ce programme permet de compenser les atténuations physiologiques de l'oreille
 * humaine en fonction de l'intensité et de la fréquence. Les calculs se basent
 * sur un niveau de mixage moyen de 80dB SPL par défaut (bientôt réglable) à
 * partir duquel la compensation s'arrête.
 * Une FFT est réalisé afin de manipuler les coefficients d'intensité des bins.
 *
 * Auteur :
 * Victor Deleau
 * Date : Version initiale le 190317, dernière modification le 270817
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <math.h>
#include <fftw3.h>
#include <time.h>

#include "wav.h"
#include "fft.h"
#include "isosonic.h"
#include "loudness.h"

int64_t *allocation(size_t size)
{
    int64_t *buffer = (int64_t *)malloc(size * sizeof(int64_t));

    if (buffer) memset(buffer, 0, size * sizeof(int64_t));

    return buffer;
}

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
    unsigned int buffer_size;
    if ((argc > 3 && sscanf(argv[3], "%u", &buffer_size) != 1) || (buffer_size < 8) || (buffer_size > 1048576))
    {
        fprintf(stderr, "Incorrect buffer size (%s)\n (8 < buffer < 1048576)\n", argv[3]);
        exit(1);
    }

    // get the listening level
    int level = 0;
    sscanf(argv[4], "%u", &level);
    if ((argc > 3) && ((level < 0) || (level > 80)))
    {
        fprintf(stderr, "Incorrect listening level (%s)\n (0 < Level < 80)\n", argv[4]);
        exit(1);
    }

    // allocate isosonic curves on the heap and load

    FILE *iso_file = NULL;
    iso_file = fopen("curve_processed.csv", "r");

    if (!iso_file)
    {
        iso_file = process_isosonic_curve();

        if (!iso_file)
        {
            fprintf(stderr, "Error while processing the isosonic curves.");
            exit(1);
        }
    }

#include "loudness.h"

    struct transfer_function transfer_function;

    transfer_function.data = (float**)malloc((2*buffer_size)*sizeof(float*));

    for (int i = 0; i < (2 * buffer_size); i++)
    {
        transfer_function.data[i] = (float*)malloc(90*sizeof(float));
    }

    for (int i = 0; i < 90; i++) transfer_function.metadata[i] = 0.;

    craft_transfer_function(iso_file, &transfer_function, &buffer_size);

    ////////////////////////////////////////////////////////////////////////////
    // 2) working buffers allocation ///////////////////////////////////////////

    // input buffer
    int64_t *input_L = allocation(buffer_size);
    int64_t *input_R = allocation(buffer_size);

    // output buffer
    int64_t *output_L = allocation(buffer_size);
    int64_t *output_R = allocation(buffer_size);

    // temp storage for P2
    float *dft_mem_L;
    float *dft_mem_R;
    dft_mem_L = malloc(sizeof(float) * (buffer_size));
    dft_mem_R = malloc(sizeof(float) * (buffer_size));

    if (input_L == NULL || input_R == NULL || output_L == NULL || output_R == NULL)
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
    unsigned int nb_lecture = (header.nb_block / (buffer_size / 2));

    // get the incomplete last buffer if needed
    if (header.nb_block % (buffer_size / 2) > 0) nb_lecture += 1;

    // hold on just a second
    char choice[3] = {0};
    printf("Go ? (y/Y/n/N)\t");
    scanf("%c", choice);
    if (choice[0] == 'n' || choice[0] == 'N') exit(1);

    printf("\n------------------------------------ Processing has started .....\n\n");

    unsigned int offset = (buffer_size/2) * header.block_size;

    size_t cumulative_read = 0, cumulative_write = 0;

    // main processing loop
    for (unsigned int i = 0; i < (nb_lecture); i++)
    {
        size_t remaining = (header.nb_block) - (cumulative_read);

        printf("------------------------------------ window n° %5u", i);

        size_t to_read = fmin(buffer_size, remaining);
        size_t count;

        // read data from disk
        count = data_read(&to_read, &header, input_L, input_R, input);

        printf(" (%zu/%zu) -----\n", count, to_read);
        cumulative_read += (count / 2);

        // apply fft + correction
        fft(&header, &buffer_size, input_L, input_R, output_L, output_R,
            dft_mem_L, dft_mem_R, &transfer_function, level);

        // write back P1 to disk
        count = data_write(&to_read, &header, output_L, output_R, output);
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

    free(input_L);
    free(input_R);

    free(dft_mem_L);
    free(dft_mem_R);

    free(output_L);
    free(output_R);

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