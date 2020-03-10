#ifndef _WAVE_H_
#define _WAVE_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef
struct Header
{
    unsigned char riff[4]; // is it PCM ? Then ok
    unsigned int file_size; // binary size
    unsigned char wave[4]; // is it a wave file ? Then ok
    char info[256]; // metadata storing, untouched
    char info2parse[256]; // metadata storing
    unsigned int info_len;  // the length of the metadata in bytes
    unsigned char fmt_chunk_marker [4]; // FMT flag
    unsigned int chunk_size; // chunk size
    unsigned int type_format; // encoding type (we want PCM)
    unsigned int nb_channel; // number of channel
    unsigned int sample_rate; // sampling frequency
    unsigned int byte_rate; // bitrate
    unsigned int block_size; // (nb_channel * chunk_size / 8)
    unsigned int bits_per_sample; // 8, 16, 24, 32 ...
    unsigned char data_chunk_header [4]; // DATA or FLLR string, data beginning
    unsigned int extra_param_len;
    unsigned int data_size; // (nb_sample * nb_channel * bits_per_sample / 8)
    unsigned long nb_block;  // number of samples
    unsigned long size_of_each_sample; // all Channel sample size
    float duration_in_seconds;
}

Header;

void display_header(Header *header);

int header_read(Header *header, FILE *input);

int data_read(size_t *buffer_size,
               Header *header,
               int64_t left[],
               int64_t right[],
               FILE *input);

int header_write(Header *header, FILE *output);

int data_write(size_t *to_read,
               Header *header,
               int64_t left[],
               int64_t write[],
               FILE *output);

long long maxint(size_t n);
long long minint(size_t n);

unsigned int buffer_to_unsigned(unsigned char *buffer, size_t size);
int buffer_to_signed(char *buffer, size_t size);
void unsigned_to_buffer(unsigned int x, unsigned char *buffer, size_t size);
void signed_to_buffer(int x, char *buffer, size_t size);

#endif // _WAVE_H_ 1
