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
 * @param BUFFER_SIZE
 * @param left channel, BUFFER_SIZE array
 * @param right channel, BUFFER_SIZE array
 * @return float
 */
/* float get_db(Header *header, unsigned int *BUFFER_SIZE, int64_t left[], int64_t right[]){
  unsigned int i = 0;
  float dbfs = 0;  //From -128 to 0dbfs
  double pmoy = 0;
  double pmax = 0;

  pmax = maxint(header->bits_per_sample); // max positive range of sample

  for (i=0; i < *BUFFER_SIZE; i++){ 
    pmoy += pow(left[i], 2);  // compute on left channel only
  }
  pmoy = sqrt( pmoy / *BUFFER_SIZE);
  dbfs = 20 * log10( pmoy / (pmax));

  printf("Buffer of %d\n", *BUFFER_SIZE);
  
  return dbfs;
} */

/** apply compensation to spectrum
 * @param BUFFER_SIZE (size_t)
 * @param left_input_spectrum (fftw_complex[])
 * @param right_input_spectrum (fftw_complex[])
 * @param left_output_spectrum (fftw_complex[])
 * @param right_output_spectrum (fftw_complex[])
 * @param transfer_function to apply (TransferFunction*)
 * @param LISTENING_LEVEL (const size_t)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t loudness(
    size_t BUFFER_SIZE,
    fftw_complex left_input_spectrum[],
    fftw_complex right_input_spectrum[],
    fftw_complex left_output_spectrum[],
    fftw_complex right_output_spectrum[],
    TransferFunction *transfer_function,
    const size_t LISTENING_LEVEL)
{

    int last = 55 - 1;
    int first = 0;
    int middle = (first + last) / 2;
    int curve2apply = 0;

    while (first <= last)
    { // binary search of the right curve to apply

        if (LISTENING_LEVEL > (transfer_function->metadata[middle] + 0.5))
        {
            first = middle + 1;
        }
        else if (LISTENING_LEVEL == (transfer_function->metadata[middle] + 0.5))
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
    for (i = 0; i < (BUFFER_SIZE / 2 + 1); i++)
    {
        // correction in dB SPL to linear scale
        to_apply = transfer_function->data[i][curve2apply] * 8;
        to_apply = pow(10, transfer_function->data[i][curve2apply] / 20);

        // apply to left channel
        left_output_spectrum[i][0] = left_input_spectrum[i][0] * to_apply;
        right_output_spectrum[i][0] = right_input_spectrum[i][0] * to_apply;

        // apply to right channel
        left_output_spectrum[i][1] = left_input_spectrum[i][1] * to_apply;
        right_output_spectrum[i][1] = right_input_spectrum[i][1] * to_apply;
    }

    return 0;
}
