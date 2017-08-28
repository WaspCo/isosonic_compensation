/* @file wave.c
 * Fichiers WAVE (definitions)
 *
 * Le format WAVE est une application des fichiers RIFF (Resource Interchange File format).
 * C'est un format de fichier conteneur generique qui permet de stocker des donnees audionumeriques
 * dans des fichiers structures en blocs, avec differents modes de compression (PCM, etc)
 *
 * Les deux fonctions header_parse et data-parse permettent de lire un fichier wav simple, structure
 * en deux blocs consecutifs :
 * - le bloc de description de contenu (header),
 * - le/les bloc(s) de stockage des donnees (data), avec des donnees non compressees (PCM) sur deux canaux.
 *
 * Source:
 * - https://en.wikipedia.org/wiki/WAV
 * Auteur:
 * Victor Deleau
 * Date: Version intiale le 220317, dernière modification le 270817
 */

#include <assert.h>    // Diagnostics (assert)
#include <inttypes.h>  // Format conversion of integer types (PRId32)
#include <math.h>      // Mathematics (fmod)
#include <stdbool.h>   // Boolean type and values (true, false)
#include <stdint.h>    // Integer types (int32_t)
#include <stdio.h>     // Input/output (FILE, fread, printf, fprintf, sprintf, stderr)
#include <stdlib.h>    // General utilities (malloc, atoi, size_t)
#include <string.h>    // String handling (strcpy, strncpy, memcmp, memset)
#include <unistd.h>    // POSIX operating system API

#include "wave.h"
#include "loudness.h"

/** Translate raw buffer (little endian bytes) to integer value
 * @param x value to translate
 * @param buffer bytes to translate
 * @param size number of bytes
 */
unsigned int buffer_to_unsigned(unsigned char *buffer, size_t size)
{
    unsigned int result = 0;
    while (size != 0) result = (result << 8) | buffer[--size];
    return result;
}

/** Translate raw buffer (little endian bytes) to signed integer value
 * @param x value to translate
 * @param buffer bytes to translate
 * @param size number of bytes
 */
int buffer_to_signed(char *buffer, size_t size)
{
    int result = 0;
    while (size != 0) result = (result << 8) | buffer[--size];
    return result;
}

/** Translate integer value to raw buffer (little endian bytes)
 * @param x value to translate
 * @param buffer bytes to translate
 * @param size number of bytes
 */
void unsigned_to_buffer(unsigned int x, unsigned char *buffer, size_t size)
{
    while (size != 0)
    {
        *buffer++ = x & 0xFFU;
        x >>= 8;
        --size;
    }
}

/** Translate signed integer value to raw buffer (little endian bytes)
 * @param x value to translate
 * @param buffer bytes to translate
 * @param size number of bytes
 */
void signed_to_buffer(int x, char *buffer, size_t size)
{
    while (size != 0)
    {
        *buffer++ = x & 0xFF;
        x >>= 8;
        --size;
    }
}

/** Displays a message containing a size
 * @param message message header
 * @param size value to display
 */
void display_size(char * message, size_t size)
{
    static const char unit[][7] = { "Bytes", "KBytes", "MBytes", "GBytes" };
    static const size_t nunit = sizeof unit / sizeof unit[0];

    size_t k;
    for (k = 0 ; k < nunit && size >= 1024 ; ++k, size /= 1024) ;

    printf("%s : %zu %s\n", message, size, unit[k]);
}

/** Display a duration in hms format
 * @param message message header
 * @param raw_secondes value to display
 **/
void display_duration(char *message, float raw_seconds)
{
    int hours = (int) raw_seconds / 3600;
    int hours_residue = (int) raw_seconds % 3600;

    int minutes = hours_residue / 60;
    int seconds = hours_residue % 60;

    int milliseconds = fmod(raw_seconds, 1.) * 1000;

    printf("%s: %d h %d m %d s %d ms (%f s)\n\n", message, hours, minutes, seconds, milliseconds, raw_seconds);
}

/** Produce min value for signed fixed-size integer
 * @param n number of bits
 * @return max value for INTn
 */
long long minint(size_t n)
{
    return n ? -(1LL << (n-1)) : 0;
}

/** Produce max value for signed fixed-size integer
 * @param n number of bits
 * @return max value for INTn
 */
long long maxint(size_t n)
{
    return n ? (1LL << (n-1)) - 1 : 0;
}

/** Display header information
 * @param header to display
 */
void display_header(Header *header)
{
    //display_size("Format bloc size", header->chunk_size);
    static const char encoding[][10] = { "? (0)", "PCM (1)", "? (2)", "? (3)", "? (4)", "? (5)", "A-Law (6)", "mu-law (7)" };
    static const unsigned nencoding = sizeof encoding / sizeof encoding[0];
    printf("Encoding: %s\n", header->type_format < nencoding ? encoding[header->type_format] : "?");
    printf("Number of channels: %u\n", header->nb_channel);
    printf("Sample rate: %u Hertz\n", header->sample_rate);
    printf("Byte Rate: %u B/s, bit rate: %u b/s\n", header->byte_rate, header->byte_rate * 8);
    printf("Sample block size: %u Bytes\n", header->block_size);
    printf("Bits per sample: %u bits\n", header->bits_per_sample);
    display_size("Data file size", header->data_size);
    printf("Number of blocks: %lu\n", header->nb_block);
    display_duration("Approx. duration", header->duration_in_seconds);
    //printf("RAW Metadata: %s\n\n", header->info2parse);
}

/** Wave file header parsing
 * @param header read data storage
 * @param input source file
 * @return 0 if an error occured, 1 otherwise
 */
int header_read(Header *header, FILE *input)
{

    // WAVE header structure
    unsigned char buffer[4];

    //Format RIFF
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading RIFF identifier failed\n");
        return 0;
    }
    if (memcmp(buffer, "RIFF", 4) != 0)
    {
        fprintf(stderr, "ERROR: RIFF file required\n");
        return 0;
    }
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading RIFF size failed\n");
        return 0;
    }

    header->file_size = buffer_to_unsigned(buffer, 4);

    //Format WAVE
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE format failed\n");
        return 0;
    }
    if (memcmp(buffer, "WAVE", 4) != 0)
    {
        fprintf(stderr, "ERROR: WAVE file required\n");
        return 0;
    }

    //Parse the possible flags
    char tmp[2];
    tmp[0] = 0;
    header->info[0] = 0;
    char fmt[4];
    fmt[0] = '\0';
    int countfmt = 3;
    strcpy(fmt, "fmt");
    unsigned char character;
    int charCount = 0, nbLetterFound = 0;
    header->info2parse[0] = '\0'; //Untouched buffer to write back

    while ((character = getc(input)) != -1 && charCount < 300){
        //ajoute le charactère a la fin du string
        header->info2parse[charCount] = character;
        header->info[charCount++] = character;

        if ((header->info[charCount-1] == '\0')
        | (header->info2parse[charCount-1] == 12)) header->info2parse[charCount-1] = 95;
        //Check if we have the right character
        if(character == fmt[nbLetterFound]){
            nbLetterFound++;
        }else{
            nbLetterFound = 0; //Else, reset
        }
        if(nbLetterFound == countfmt){
            break;  //If we have the right number of caracters (3), stop
        }
    }

    // Cut the fmt flag and get the right lenght
    if (charCount > 3)
    {
      header->info[charCount-4] = '\0';
      header->info2parse[charCount-4] = '\0';
      header->info_len = charCount-3;
    }
    else {  //Pas de metadata
      header->info[0] = '\0';
      header->info2parse[0] = '\0';
      header->info_len = 0;
    }

    fread(tmp, 1, 1, input);  //Eat the next empty byte

    // (16-19) Wave format: subchunk size
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading subchunk size failed\n");
        return 0;
    }
    header->chunk_size = buffer_to_unsigned(buffer, 4);

    // (20-21) Wave format: encoding
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE encoding failed\n");
        return 0;
    }
    header->type_format = buffer_to_unsigned(buffer, 2);
    if (header->type_format != 1)
    {
        fprintf(stderr, "ERROR: Encoding flag is not PCM. This program can't handle compression.\n");
        return 0;
    }
    //printf("%d\n",header->type_format);

    // (22-23) Wave format: number of channels
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE number of channels failed\n");
        return 0;
    }
    header->nb_channel = buffer_to_unsigned(buffer, 2);
    //printf("%d\n",header->nb_channel);
    if ((header->nb_channel != 1) && (header->nb_channel != 2))
    {
        fprintf(stderr, "ERROR: This program can only handle 1 or 2 channels\n");
        return 0;
    }

    // (24-27) Wave format: sample rate
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE sample rate failed\n");
        return 0;
    }
    header->sample_rate = buffer_to_unsigned(buffer, 4);
    //printf("%d\n",header->sample_rate);

    // (28-31) Wave format: byte rate
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE byte rate failed\n");
        return 0;
    }
    header->byte_rate = buffer_to_unsigned(buffer, 4);
    //printf("%d\n",header->byte_rate);

    // (32-33) Wave format: sample bloc size
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE sample block size failed\n");
        return 0;
    }
    header->block_size = buffer_to_unsigned(buffer, 2);
    //printf("%d\n",header->block_size);

    // (34-35) Wave format: bits per sample
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE bits per sample failed\n");
        return 0;
    }
    header->bits_per_sample = buffer_to_unsigned(buffer, 2);
    //printf("%d\n",header->bits_per_sample);

    // (36-39) Wave data bloc header: "data" data bloc identifier
    unsigned char data[5];
    memset(data, 0, 5);
    data[0] = '\0';
    int i = 0;

    tmp[0] = 0;

    if (fread(tmp, 1, 1, input) != 1)
    {
       fprintf(stderr, "ERROR: reading data size failed\n");
       return 0;
    }

    data[0] = data[1];
    data[1] = data[2];
    data[2] = data[3];
    data[3] = tmp[0];
    data[4] = '\0';

    while ((memcmp(data, "data", 4) != 0) && i<300)
    {
        if (fread(tmp, 1, 1, input) != 1)
        {
            fprintf(stderr, "ERROR: reading data size failed\n");
            return 0;
        }
        i++;

        data[0] = data[1];
        data[1] = data[2];
        data[2] = data[3];
        data[3] = tmp[0];
        data[4] = '\0';

        memset(tmp, 0, 1);
    }

    //Extra parameter lenght
    header->extra_param_len = i - 3;

    // (40-43) Wava data bloc header: data bloc size
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading data block size failed\n");
        return 0;
    }
    header->data_size = buffer_to_unsigned(buffer, 4);

    // number of samples
    assert(header->nb_channel * header->bits_per_sample == header->block_size * 8);
    header->nb_block = (8 * (header->data_size)) / (header->nb_channel * header->bits_per_sample);

    // size of sample
    header->size_of_each_sample = (header->nb_channel * header->bits_per_sample) / 8;

    // duration of file
    header->duration_in_seconds = (float)header->file_size / header->byte_rate;

    return 0;
}

/** Wave file data parsing & processing
 * @param buffer_size
 * @param input_L new data (left)
 * @param input_R new data (right)
 * @param ptr to input source file
 * @return number of sample block read
 */
int data_read(size_t *to_read,
               Header *header,
               int64_t left[],
               int64_t right[],
               FILE *input)
               {

  // Check header consistency
  size_t bytes_in_each_channel = ((header->size_of_each_sample) / (header->nb_channel));
  if (bytes_in_each_channel * header->nb_channel != header->size_of_each_sample)
  {
    fprintf(stderr,
      "Format error: sample size does not match header information (%ld x %ud # %ld)\n",
      bytes_in_each_channel,
      header->nb_channel,
      header->size_of_each_sample);
      return 0;
  }

  short int data_buffer;

  for (unsigned buffer_sample = 0; buffer_sample < (*to_read); buffer_sample++)
  {
      // Left channel
      if (fread(&data_buffer, sizeof data_buffer, 1, input) != 1){
          fprintf(stderr, "Error while reading sample %u (left)\n", buffer_sample);
          return buffer_sample;
      }
      //left[buffer_sample] = buffer_to_signed(data_buffer, header->bits_per_sample/8);
      left[buffer_sample] = data_buffer;

      // Right channel
      if (fread(&data_buffer, sizeof data_buffer, 1, input) != 1)
      {
          fprintf(stderr, "Error while reading sample %u (right)\n", buffer_sample);
          return buffer_sample;
      }
      //right[buffer_sample] = buffer_to_signed(data_buffer, header->bits_per_sample/8);
      right[buffer_sample] = data_buffer;
  }

  return *to_read;
}


/** Write Wave file header
 * @param wave header
 * @param pt to output file
 *
 * Write the beginning of a simple Wave file (44 bytes at least).
 */
int header_write(Header *header, FILE *output)
{
    unsigned char buffer[4];

    fwrite("RIFF", 4, 1, output);

    unsigned_to_buffer(header->file_size, buffer, 4);
    fwrite(buffer, 4, 1, output);

    fwrite("WAVE", 4, 1, output);

    fwrite(header->info, header->info_len, 1, output);

    fwrite("fmt ", 4, 1, output);

    unsigned_to_buffer(header->chunk_size, buffer, 4);
    fwrite(buffer, 4, 1, output);

    unsigned_to_buffer(header->type_format, buffer, 2);
    fwrite(buffer, 2, 1, output);

    unsigned_to_buffer(header->nb_channel, buffer, 2);
    fwrite(buffer, 2, 1, output);

    unsigned_to_buffer(header->sample_rate, buffer, 4);
    fwrite(buffer, 4, 1, output);

    unsigned_to_buffer(header->byte_rate, buffer, 4);
    fwrite(buffer, 4, 1, output);

    unsigned_to_buffer(header->block_size, buffer, 2);
    fwrite(buffer, 2, 1, output);

    unsigned_to_buffer(header->bits_per_sample, buffer, 2);
    fwrite(buffer, 2, 1, output);

    for(int i=0; i<(header->extra_param_len); i++){
      fwrite("\0", 1, 1, output); //Extra parameter
    }
    fwrite("data", 4, 1, output);


    unsigned_to_buffer(header->data_size, buffer, 4);
    fwrite(buffer, 4, 1, output);

    return 1;
}

/** Write Wave file data
 * @param buffer_size
 * @param wave header
 * @param left channel buffer
 * @param right channel buffer
 * @param ptr to output file
 * @return 0 if an error occured, 1 otherwise
 *
 *  Write one buffer of samples (block_size * buffer_size)
 */
int data_write(size_t *to_read,
               Header *header,
               int64_t left[],
               int64_t right[],
               FILE *output){

  char b[2] = {'\0'};
  for (unsigned i = 0 ; i < (*to_read/2) ; ++i)
  {
    // Left channel
    signed_to_buffer(left[i], b, ((header->bits_per_sample)/8));
    if (fwrite(b, ((header->bits_per_sample)/8), 1, output) != 1){
        fprintf(stderr, "Error while writing sample %u (left)\n", i);
        return i;
    }
    // Right channel
    signed_to_buffer(right[i], b, ((header->bits_per_sample)/8));
    if (fwrite(b, ((header->bits_per_sample)/8), 1, output) != 1){
        fprintf(stderr, "Error while writing sample %u (right)\n", i);
        return i;
    }
  }
  return *to_read;
}
