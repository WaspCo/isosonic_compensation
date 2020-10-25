/* @file loudness.c
 * Correction des courbes isophoniques
 *
 * Les courbes isophoniques (Fletcher & Munson et leurs améliorations) représentent
 * la sensibilité de l'oreille humaine en fonction du niveau et de la fréquence.
 * Elles permettent de définir le phone, qui est une échelle
 * d'intensité sonore relative à la perception de l'oreille humaine.
 *
 * Source:
 * - https://en.wikipedia.org/wiki/Equal-loudness_contour
 * Auteurs:
 * Victor Deleau
 * Date: Version intiale le 010417, dernière modification le 270817
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include <fftw3.h>

#include "isosonic.h"
#include "loudness.h"
#include "wav.h"


/** Get dbFullScale value out of sample value
 * This is a draft, not used for now
 * @param header informations
 * @param buffer_size
 * @param left channel, buffer_size array
 * @param right channel, buffer_size array
 * @return float
 */
/* float get_db(Header *header, unsigned int *buffer_size, int64_t left[], int64_t right[]){
  unsigned int i = 0;
  float dbfs = 0;  //From -128 to 0dbfs
  double pmoy = 0;
  double pmax = 0;

  pmax = maxint(header->bits_per_sample); // max positive range of sample

  for (i=0; i < *buffer_size; i++){ 
    pmoy += pow(left[i], 2);  // compute on left channel only
  }
  pmoy = sqrt( pmoy / *buffer_size);
  dbfs = 20 * log10( pmoy / (pmax));

  printf("Buffer of %d\n", *buffer_size);
  
  return dbfs;
} */

/** apply compensation to spectrum
 * @param header informations
 * @param buffer_size
 * @param left channel spectrum
 * @param right channel spectrum
 * @param left channel result of frequency product
 * @param right channel result of frequency product
 * @param curve_processed, isophonic curves
 * @param listening level
 * @return integer
 */
int loudness(
    Header *header,
    unsigned int *buffer_size,
    fftw_complex dft_freq_L[],
    fftw_complex dft_freq_R[],
    fftw_complex dft_freq_conv_L[],
    fftw_complex dft_freq_conv_R[],
    struct transfer_function *curve_processed,
    int level)
{

    int last = 55 - 1;
    int first = 0;
    int middle = (first + last) / 2;
    int curve2apply = 0;

    while (first <= last)
    { // binary search of the right curve to apply

        if (level > (curve_processed->metadata[middle] + 0.5))
        {
            first = middle + 1;
        }
        else if (level == (curve_processed->metadata[middle] + 0.5))
        {
            break;
        }
        else last = middle - 1;

        middle = (first + last) / 2;
    }

    curve2apply = middle + 1;

    int i = 0;
    float to_apply = 0;

    // apply correction to samples
    for (i = 0; i < (*buffer_size / 2 + 1); i++)
    {
        // correction in dB SPL to linear scale
        to_apply = curve_processed->data[i][curve2apply] * 8;
        to_apply = pow(10, curve_processed->data[i][curve2apply] / 20);

        // apply to left channel
        dft_freq_conv_L[i][0] = dft_freq_L[i][0] * to_apply;
        dft_freq_conv_R[i][0] = dft_freq_R[i][0] * to_apply;

        // apply to right channel
        dft_freq_conv_L[i][1] = dft_freq_L[i][1] * to_apply;
        dft_freq_conv_R[i][1] = dft_freq_R[i][1] * to_apply;
    }

    return 0;
}
