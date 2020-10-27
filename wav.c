/* @file wave.c
 * Read and write WAV audio files
 *
 * The two main functions are:
 * - header_parse() which allows to parse the WAV file header
 * - data_parse() which allows to read the audio data
 *
 * Source:
 * - https://en.wikipedia.org/wiki/WAV
 * Auteur:
 * Victor Deleau
 */

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wav.h"
#include "loudness.h"

/** Translate raw buffer (little endian bytes) to integer value
 * @param buffer of bytes to translate (unsigned char*)
 * @param size number of bytes (size_t)
 * @return (unsigned int)
 */
unsigned int buffer_to_unsigned(unsigned char *buffer, size_t size)
{
    unsigned int result = 0;

    while (size != 0)
        result = (result << 8) | buffer[--size];

    return result;
}

/** Translate raw buffer (little endian bytes) to signed integer value
 * @param buffer bytes to translate (char*)
 * @param size number of bytes (size_t)
 * @return (int)
 */
int buffer_to_signed(char *buffer, size_t size)
{
    int result = 0;

    while (size != 0)
        result = (result << 8) | buffer[--size];

    return result;
}

/** Translate integer value to raw buffer (little endian bytes)
 * @param x value to translate (unsigned int)
 * @param buffer of bytes to translate (unsigned char*)
 * @param size number of bytes (size_t)
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
 * @param x value to translate (int)
 * @param buffer of bytes to translate (char*)
 * @param size number of bytes (size_t)
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

/** Displays the size of the message in bytes
 * @param message header (char*)
 * @param size value to display (size_t)
 */
void display_size(char *message, size_t size)
{
    static const char unit[][7] = {"Bytes", "KBytes", "MBytes", "GBytes"};
    static const size_t nunit = sizeof unit / sizeof unit[0];

    size_t k;
    for (k = 0; k < nunit && size >= 1024; ++k, size /= 1024)
        ;

    printf("%s : %zu %s\n", message, size, unit[k]);
}

/** Display the duration in hms format
 * @param message header (char*)
 * @param raw_seconds value to display (float)
 **/
void display_duration(char *message, float raw_seconds)
{
    int hours = (int)raw_seconds / 3600;
    int hours_residue = (int)raw_seconds % 3600;

    int minutes = hours_residue / 60;
    int seconds = hours_residue % 60;

    int milliseconds = fmod(raw_seconds, 1.) * 1000;

    printf("%s: %d h %d m %d s %d ms (%f s)\n\n", message, hours, minutes, seconds, milliseconds, raw_seconds);
}

/** Produce min value for signed fixed-size integer
 * @param n number of bits (size_t)
 * @return max value for n (long long)
 */
long long minint(size_t n)
{
    return n ? -(1LL << (n - 1)) : 0;
}

/** Produce max value for signed fixed-size integer
 * @param n number of bits (size_t)
 * @return max value for n (long long)
 */
long long maxint(size_t n)
{
    return n ? (1LL << (n - 1)) - 1 : 0;
}

/** Display WAV file header information
 * @param header to display (struct header)
 */
void display_header(Header *header)
{
    static const char encoding[][10] = {"? (0)", "PCM (1)", "? (2)", "? (3)", "? (4)", "? (5)", "A-Law (6)", "mu-law (7)"};

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
}

/** parse WAV file header
 * @param header (struct header*)
 * @param input source file (FILE*)
 * @return 0 if an error occured, 1 otherwise (int)
 */
uint8_t header_read(Header *header, FILE *input)
{
    // WAVE header structure
    unsigned char buffer[4];

    // RIFF format
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading RIFF identifier failed.\n");
        return 1;
    }
    if (memcmp(buffer, "RIFF", 4) != 0)
    {
        fprintf(stderr, "ERROR: RIFF file required.\n");
        return 1;
    }
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading RIFF size failed.\n");
        return 1;
    }

    header->file_size = buffer_to_unsigned(buffer, 4);

    // WAVE format
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE format failed.\n");
        return 1;
    }
    if (memcmp(buffer, "WAVE", 4) != 0)
    {
        fprintf(stderr, "ERROR: WAVE file required.\n");
        return 1;
    }

    // parse the possible flags
    char tmp[2];
    tmp[0] = 0;
    header->info[0] = 0;
    char fmt[4];
    fmt[0] = '\0';
    int countfmt = 3;
    strcpy(fmt, "fmt");
    unsigned char character;
    int charCount = 0, nbLetterFound = 0;
    header->info2parse[0] = '\0'; // untouched buffer to write back

    while ((character = getc(input)) != -1 && charCount < 300)
    {
        header->info2parse[charCount] = character;
        header->info[charCount++] = character;

        if ((header->info[charCount - 1] == '\0') | (header->info2parse[charCount - 1] == 12))
            header->info2parse[charCount - 1] = 95;

        // check if we have the right character
        if (character == fmt[nbLetterFound])
            nbLetterFound++;
        else
            nbLetterFound = 0; // else reset

        if (nbLetterFound == countfmt)
            break;
    }

    // cut the fmt flag and get the right lenght
    if (charCount > 3)
    {
        header->info[charCount - 4] = '\0';
        header->info2parse[charCount - 4] = '\0';
        header->info_len = charCount - 3;
    }
    else
    { // no metadata available
        header->info[0] = '\0';
        header->info2parse[0] = '\0';
        header->info_len = 0;
    }

    fread(tmp, 1, 1, input); // eat the next empty byte

    // (16-19) Wave format: subchunk size
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading subchunk size failed.\n");
        return 1;
    }

    header->chunk_size = buffer_to_unsigned(buffer, 4);

    // (20-21) Wave format: encoding
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE encoding failed.\n");
        return 1;
    }

    header->type_format = buffer_to_unsigned(buffer, 2);

    if (header->type_format != 1)
    {
        fprintf(stderr, "ERROR: Encoding flag is not PCM. This program can't handle compressed audio files.\n");
        return 1;
    }

    // (22-23) Wave format: number of channels
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE number of channels failed.\n");
        return 1;
    }

    header->nb_channel = buffer_to_unsigned(buffer, 2);

    if ((header->nb_channel != 1) && (header->nb_channel != 2))
    {
        fprintf(stderr, "ERROR: This program can only handle 1 or 2 channels.\n");
        return 1;
    }

    // (24-27) Wave format: sample rate
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE sample rate failed.\n");
        return 1;
    }

    header->sample_rate = buffer_to_unsigned(buffer, 4);

    // (28-31) Wave format: byte rate
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE byte rate failed.\n");
        return 1;
    }

    header->byte_rate = buffer_to_unsigned(buffer, 4);

    // (32-33) Wave format: sample bloc size
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE sample block size failed.\n");
        return 1;
    }

    header->block_size = buffer_to_unsigned(buffer, 2);

    // (34-35) Wave format: bits per sample
    if (fread(buffer, 2, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading WAVE bits per sample failed.\n");
        return 1;
    }

    header->bits_per_sample = buffer_to_unsigned(buffer, 2);

    // (36-39) Wave data bloc header: "data" data bloc identifier
    unsigned char data[5];
    memset(data, 0, 5);
    data[0] = '\0';
    int i = 0;

    tmp[0] = 0;

    if (fread(tmp, 1, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading data size failed.\n");
        return 1;
    }

    data[0] = data[1];
    data[1] = data[2];
    data[2] = data[3];
    data[3] = tmp[0];
    data[4] = '\0';

    while ((memcmp(data, "data", 4) != 0) && i < 300)
    {
        if (fread(tmp, 1, 1, input) != 1)
        {
            fprintf(stderr, "ERROR: reading data size failed.\n");
            return 1;
        }
        i++;

        data[0] = data[1];
        data[1] = data[2];
        data[2] = data[3];
        data[3] = tmp[0];
        data[4] = '\0';

        memset(tmp, 0, 1);
    }

    header->extra_param_len = i - 3; // extra parameter lenght

    // (40-43) Wava data bloc header: data bloc size
    if (fread(buffer, 4, 1, input) != 1)
    {
        fprintf(stderr, "ERROR: reading data block size failed.\n");
        return 1;
    }

    header->data_size = buffer_to_unsigned(buffer, 4);

    // get number of samples
    assert(header->nb_channel * header->bits_per_sample == header->block_size * 8);
    header->nb_block = (8 * (header->data_size)) / (header->nb_channel * header->bits_per_sample);

    // get size of sample
    header->size_of_each_sample = (header->nb_channel * header->bits_per_sample) / 8;

    // get audio file duration
    header->duration_in_seconds = (float)header->file_size / header->byte_rate;

    return 0;
}

/** Write WAV file header (44 bytes or more)
 * @param header (struct header*)
 * @param output_file (FILE*)
 */
uint8_t header_write(Header *header, FILE *output_file)
{
    unsigned char buffer[4];

    fwrite("RIFF", 4, 1, output_file);

    unsigned_to_buffer(header->file_size, buffer, 4);
    fwrite(buffer, 4, 1, output_file);

    fwrite("WAVE", 4, 1, output_file);

    fwrite(header->info, header->info_len, 1, output_file);

    fwrite("fmt ", 4, 1, output_file);

    unsigned_to_buffer(header->chunk_size, buffer, 4);
    fwrite(buffer, 4, 1, output_file);

    unsigned_to_buffer(header->type_format, buffer, 2);
    fwrite(buffer, 2, 1, output_file);

    unsigned_to_buffer(header->nb_channel, buffer, 2);
    fwrite(buffer, 2, 1, output_file);

    unsigned_to_buffer(header->sample_rate, buffer, 4);
    fwrite(buffer, 4, 1, output_file);

    unsigned_to_buffer(header->byte_rate, buffer, 4);
    fwrite(buffer, 4, 1, output_file);

    unsigned_to_buffer(header->block_size, buffer, 2);
    fwrite(buffer, 2, 1, output_file);

    unsigned_to_buffer(header->bits_per_sample, buffer, 2);
    fwrite(buffer, 2, 1, output_file);

    for (int i = 0; i < (header->extra_param_len); i++)
    {
        fwrite("\0", 1, 1, output_file); // extra parameters
    }
    fwrite("data", 4, 1, output_file);

    unsigned_to_buffer(header->data_size, buffer, 4);
    fwrite(buffer, 4, 1, output_file);

    return 0;
}

/** Read WAV file data block of samples
 * @param bytes_to_read (size_t)
 * @param left_input_buffer to read into (int64_t[])
 * @param right_input_buffer to read into (int64_t[])
 * @param input_file to read from (FILE*)
 * @return number of bytes read (int)
 */
int data_read(size_t bytes_to_read,
              Header *header,
              int64_t left_input_buffer[],
              int64_t right_input_buffer[],
              FILE *input_file)
{
    // check header consistency
    size_t bytes_in_each_channel = ((header->size_of_each_sample) / (header->nb_channel));

    if (bytes_in_each_channel * header->nb_channel != header->size_of_each_sample)
    {
        fprintf(stderr,
                "Format error: sample size does not match header information (%ld x %ud # %ld).\n",
                bytes_in_each_channel,
                header->nb_channel,
                header->size_of_each_sample);

        return 1;
    }

    short int data_buffer = 0;

    for (unsigned i = 0; i < bytes_to_read; i++)
    {
        // left channel
        if (fread(&data_buffer, sizeof data_buffer, 1, input_file) != 1)
        {
            fprintf(stderr, "Error while reading sample %u (left).\n", i);
            return -1;
        }
        left_input_buffer[i] = data_buffer;

        // right channel
        if (fread(&data_buffer, sizeof data_buffer, 1, input_file) != 1)
        {
            fprintf(stderr, "Error while reading sample %u (right).\n", i);
            return -1;
        }
        right_input_buffer[i] = data_buffer;
    }

    return bytes_to_read;
}

/** Write Wave file data
 * @param bytes_to_write (size_t)
 * @param header (header*)
 * @param left_buffer to write (int64_t[])
 * @param right_buffer to write (int64_t[])
 * @param output_file (FILE*)
 * @return number of bytes to write (int)
 */
int data_write(size_t bytes_to_write,
               Header *header,
               int64_t left_buffer[],
               int64_t right_buffer[],
               FILE *output_file)
{

    char b[2] = {'\0'};
    for (unsigned i = 0; i < (bytes_to_write / 2); ++i)
    {
        // left channel
        signed_to_buffer(left_buffer[i], b, ((header->bits_per_sample) / 8));
        if (fwrite(b, ((header->bits_per_sample) / 8), 1, output_file) != 1)
        {
            fprintf(stderr, "Error while writing sample %u (left).\n", i);
            return -1;
        }

        // right channel
        signed_to_buffer(right_buffer[i], b, ((header->bits_per_sample) / 8));
        if (fwrite(b, ((header->bits_per_sample) / 8), 1, output_file) != 1)
        {
            fprintf(stderr, "Error while writing sample %u (right).\n", i);
            return -1;
        }
    }

    return bytes_to_write;
}
