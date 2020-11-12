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

/** apply transfer function to spectrum in place
 * @param spectrum (fftw_complex[])
 * @param transfer_function to apply (const TransferFunction)
 * @param NB_BINS (const size_t)
 * @return 0 if ok, 1 otherwise (uint8_t)
 */
uint8_t spectrum_product(
    fftw_complex spectrum[],
    const TransferFunction *transfer_function,
    const size_t NB_BINS)
{    

    for (size_t i = 0; i < NB_BINS; i++)
    {
        spectrum[i][0] *= transfer_function->data[i];
        spectrum[i][1] *= transfer_function->data[i];
    }

    return 0;
}


/** Find the right transfer fucntion to apply
 * @param transfer_function to apply (TransferFunction**)
 * @param LISTENING_LEVEL (const size_t)
 * @return the index of the transfer function to apply (size_t)
 */
size_t get_curve_from_listening_level(
    TransferFunction **transfer_functions,
    size_t const LISTENING_LEVEL)
{
    size_t last = 55 - 1;
    size_t first = 0;
    size_t middle = (first + last) / 2;
    size_t curve2apply = 0;

    float current_level = transfer_functions[middle]->level_at_1000hz + 0.5;

    // binary search
    while (first <= last)
    { 
        if (LISTENING_LEVEL > current_level)
        {
            first = middle + 1;
        }
        else if (LISTENING_LEVEL == current_level)
        {
            break;
        }
        else last = middle - 1;

        middle = (first + last) / 2;
    }

    return middle + 1;
}