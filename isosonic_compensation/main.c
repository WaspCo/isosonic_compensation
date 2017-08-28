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

#include "wave.h"
#include "fft.h"
#include "gui.h"

int64_t *allocation(size_t size)
{
    int64_t *buffer = (int64_t *)malloc(size * sizeof (int64_t));
    if (buffer)   // Zero filling of the buffer
        memset(buffer, 0, size * sizeof (int64_t));
    return buffer;
}

int main (int argc, char *argv[])
{
    clock_t begin = clock();  // Ellapsed time

    printf("---------------------------------------------------------------------\n");
    printf("------------------------------------ Isosonic Compensation ----------\n");
    printf("---------------------------------------------------------------------\n\n");

    // 1) I/O //////////////////////////////////////////////////////////////////

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s [input_file]* [output_file]* [buffer_size]* [listening_level]\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL)
    {
        fprintf(stderr, "Error while opening the input file. (%s)\n", argv[1]);
        return 1;
    }
    FILE *output = fopen(argv[2], "w");
    if (output == NULL)
    {
        fprintf(stderr, "Error while allocating the output file. (%s)\n", argv[2]);
        return 1;
    }

    FILE *iso_file = fopen("c_matrice.csv", "r");
    if (iso_file == NULL)
    {
        fprintf(stderr, "Error while loading the isophonic curves data.\n");
        return 1;
    }

    Header header;  // Reading of the header
    header_read(&header, input);

    unsigned int buffer_size; // Get the buffer size specified
    if ( (argc > 3 && sscanf(argv[3], "%u", &buffer_size) != 1) || (buffer_size < 8) || (buffer_size > 1048576) )
    {
        fprintf(stderr, "Incorrect buffer size (%s)\n (8 < buffer < 1048576)\n", argv[3]);
        return 1;
    }

    int level = 0;  // Get the listening level
    sscanf(argv[4], "%u", &level);
    if ( (argc > 3) && ((level<0) || (level>80)) )
    {
        fprintf(stderr, "Incorrect listening level (%s)\n (0 < Level < 80)\n", argv[4]);
        return 1;
    }

    // Dynamic allocation of the isophonic curves data
    // The original .CSV is transposed
    #include "loudness.h"

    c_temp.c = (float**)malloc(31 * sizeof(float*));
    c_matrice.c = (float**)malloc((2*buffer_size) * sizeof(float*));

    for(int i = 0; i < 31; i++)
      { c_temp.c[i] = (float*)malloc(90 * sizeof(float)); }

    for(int i = 0; i < (2*buffer_size); i++)
      { c_matrice.c[i] = (float*)malloc(90 * sizeof(float)); }

    // Isophonic curves parsing
    load_isophonic(iso_file, &c_matrice, &c_temp, &buffer_size);

    // 2) Working buffers allocation ///////////////////////////////////////////

    // Input buffer
    int64_t *input_L = allocation(buffer_size);
    int64_t *input_R = allocation(buffer_size);
    // Output buffer
    int64_t *output_L = allocation(buffer_size);
    int64_t *output_R = allocation(buffer_size);

    float *dft_mem_L; // Temporary storage of P2
    float *dft_mem_R;
    dft_mem_L = malloc( sizeof (float) * (buffer_size));
    dft_mem_R = malloc( sizeof (float) * (buffer_size));

    if ( input_L == NULL
            || input_R == NULL
            || output_L == NULL
            || output_R == NULL )
    {
        fprintf(stderr,"Error while allocating the working buffers.\n");
        return 1;
    }

    // 3) Parsing / Processing / Writing ///////////////////////////////////////

    header_write(&header, output);
    display_header(&header);

    //How much iteration of the processing loop we should do
    unsigned int nb_lecture = (header.nb_block / (buffer_size/2));
    // To get the incomplete last buffer
    if (header.nb_block % (buffer_size/2) > 0) nb_lecture += 1;

    char choice[3] = {0}; //Hold on just a second
    printf("Go ? (y/n)\t");
    scanf("%c", choice);

    printf("\n------------------------------------ Starting convolution engine .....\n\n");
    unsigned int offset = (buffer_size/2) * header.block_size;

    //Main processing loop
    size_t cumulative_read = 0, cumulative_write = 0;
    for (unsigned int i = 0 ; i < (nb_lecture) ; i++)
    {
            size_t remaining = (header.nb_block) - (cumulative_read);
            printf("------------------------------------ buffer n° %5u", i);
            size_t to_read = fmin(buffer_size,remaining);
            size_t count;


            count = data_read(&to_read, &header, input_L, input_R, input);
            printf(" (%zu/%zu) -----\n", count, to_read);
            cumulative_read += (count/2);

            fft(&header, &buffer_size, input_L, input_R,
              output_L, output_R, dft_mem_L, dft_mem_R, level);

            count = data_write(&to_read, &header, output_L, output_R, output);
            cumulative_write += (count/2);

            // Overlapp-add, rewind pointer of offset bytes
            long int input_p = ftell(input);
            fseek(input,input_p - offset, SEEK_SET);
    }

    printf("------------------------------------ Alt er vel (^_^) ----------------\n");
    printf("Sample read: %zu\n", cumulative_read);
    printf("Sample writen: %zu\n", cumulative_write);  // End of the audio processing

    //Updating the chunk size (nb file bytes size - 8)
    unsigned char buffer[4];
    fseek(output, 0L, SEEK_END);
    int new_chunk_size = ftell(output) - 8;
    unsigned_to_buffer(new_chunk_size, buffer, 4);
    fseek(output, 4, SEEK_SET);
    fwrite(buffer, 4, 1, output);

    //Updating the sub chunk size 2 (nb bytes in the data)
    int new_sub_chunk_size2 = new_chunk_size - 36 - (header.info_len) - (header.extra_param_len);
    unsigned_to_buffer(new_sub_chunk_size2, buffer, 4);
    fseek(output, 40+(header.info_len)+(header.extra_param_len), SEEK_SET);
    fwrite(buffer, 4, 1, output);

    // Now let's free the memory
    for (int i = 0; i < 31; i++) {  free(c_temp.c[i]);  }
    free(c_temp.c);
    for (int i = 0; i < (2*buffer_size); i++) {  free(c_matrice.c[i]);  }
    free(c_matrice.c);

    free(input_L);
    free(input_R);
    free(dft_mem_L);
    free(dft_mem_R);
    free(output_L);
    free(output_R);

    if (fclose(input))  //File closing
    {
        printf("Error while closing the input file.");
        exit(-1);
    }
    if (fclose(output))
    {
        printf("Error while closing the output file.");
        exit(-1);
    }

    clock_t end = clock();  // Get the final ellapsed time
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time: %lf seconds\n", time_spent);

    return 0;
// END
}
