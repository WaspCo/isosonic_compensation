/* @file fft.c
 * Transformé de Fourier Discrete sur tableau de sample (definitions)
 *
 * Source:
 * - https://en.wikipedia.org/wiki/Discrete_Fourier_transform
 * Auteur:
 * Victor Deleau
 * Date: Version intiale le 220317, dernière modification le 270817
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <fftw3.h>

#include "fft.h"
#include "loudness.h"

#define REAL 0
#define IMAG 1

/** Perform a circular shifting of an array
 * @param left sample block
 * @param right sample block
 * @param buffer size
 *  The array of sample is shifted by buffer_size/2
 */
double circshift(double buffer_L[], double buffer_R[], unsigned int *size){
  double circ_L[*size/2];
  double circ_R[*size/2];
  int i = 0;
  for(i = 0; i < (*size/2-1); i++)
  {
    circ_L[i] = buffer_L[i];
    circ_R[i] = buffer_R[i];
  }
  for(i = 0; i < (*size/2-1); i++)
  {
    buffer_L[i] = buffer_L[i+(*size/2)];
    buffer_R[i] = buffer_R[i+(*size/2)];
    buffer_L[i+(*size/2)] = circ_L[i];
    buffer_R[i+(*size/2)] = circ_R[i];
  }
  return 1;
}


/** Perform a FFT on an array of samples
 * @param header
 * @param buffer_size, number of block in each buffer
 * @param left input buffer
 * @param right input buffer
 * @param left output buffer
 * @param right output buffer
 * @param memory output buffer
 * @param memory output buffer
 * @param listening level
 * @return 0 if an error occured, 1 otherwise
 */
double fft(Header *header, unsigned int *buffer_size, int64_t input_L[], int64_t input_R[],
 					 int64_t output_L[], int64_t output_R[], float *dft_mem_L, float *dft_mem_R, int level)
	{
	// Variable Declaration
	int n = *buffer_size;
  int fft_length = (n / 2 ) + 1;
	int i;
  //double pi = 3.14159265359;

  //Time audio samples INPUT
	double *dft_time_L;
	double *dft_time_R;

  //Freq audio samples FFT OUTPUT
	fftw_complex *dft_freq_L;
	fftw_complex *dft_freq_R;

  //Freq audio samples PRODUCT RESULT
	fftw_complex *dft_freq_conv_L;
	fftw_complex *dft_freq_conv_R;

	// Memory allocation
	dft_time_L = malloc( sizeof (double) * n);
	dft_time_R = malloc( sizeof (double) * n);

	dft_freq_L = fftw_malloc( sizeof (fftw_complex) * fft_length);
	dft_freq_R = fftw_malloc( sizeof (fftw_complex) * fft_length);

  dft_freq_conv_L = fftw_malloc( sizeof (fftw_complex) * fft_length);
	dft_freq_conv_R = fftw_malloc( sizeof (fftw_complex) * fft_length);

	//Data in // Buffers filling
  for(i = 0; i < n; i++)  // P1 filling
  {
    dft_time_L[i] = (double)input_L[i] / 0x8000;// * 0.5 * (1 - cos(2*pi*i/(n*2-1)));		// test hann window;
    dft_time_R[i] = (double)input_R[i] / 0x8000;// * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
    //printf("%f\n", dft_time_L[i]);
  }

  //circshift
  circshift(dft_time_L, dft_time_R, buffer_size);

	// FFTW_FORWARD
  fftw_plan plan_forward;		// FFTW plans // Parameters
  fftw_plan plan_backward;

	plan_forward = fftw_plan_dft_r2c_1d(fft_length, dft_time_L, dft_freq_L,
    FFTW_MEASURE);
	fftw_execute(plan_forward);	// Left channel
	plan_forward = fftw_plan_dft_r2c_1d (fft_length, dft_time_R, dft_freq_R,
    FFTW_MEASURE);
	fftw_execute(plan_forward); // Right channel

  // Compensation
  loudness(header, buffer_size, dft_freq_L, dft_freq_R, dft_freq_conv_L, dft_freq_conv_R, &c_matrice, level);

  // FFTW_BACKWARD
	plan_backward = fftw_plan_dft_c2r_1d(fft_length, dft_freq_conv_L,
    dft_time_L, FFTW_MEASURE);
	fftw_execute(plan_backward);  // Left channel
	plan_backward = fftw_plan_dft_c2r_1d(fft_length, dft_freq_conv_R,
    dft_time_R, FFTW_MEASURE);
	fftw_execute(plan_backward); // Right channel

  //circshift
  circshift(dft_time_L, dft_time_R, buffer_size);

  // Convert it back to integer and hann windows
  for(i=0; i < n; i++)
		{
		dft_time_L[i] = dft_time_L[i] / ((2*n)-1) * 0x8000;// * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
    dft_time_R[i] = dft_time_R[i] / ((2*n)-1) * 0x8000;// * 0.5 * (1 - cos(2*pi*i/(n*2-1)));
	}

	for(i=0; i < (n/2); i++) // Add new P1 to old P2
		{
		output_L[i] = dft_mem_L[i] + dft_time_L[i];
		output_R[i] =  dft_mem_R[i] + dft_time_R[i];
	}

  // OVERLAPP-ADD memorisation
  int j = 0;  // New P2 storage
  for (int i = (n/2); i < n; i++ ) {
    dft_mem_L[j] =  dft_time_L[i];
    dft_mem_R[j] =  dft_time_R[i];
    j++;
  }

	fftw_destroy_plan(plan_forward);  //Libération mémoire
	fftw_destroy_plan(plan_backward);
	fftw_free(dft_time_L);
	fftw_free(dft_time_R);
	fftw_free(dft_freq_L);
	fftw_free(dft_freq_R);
	fftw_free(dft_freq_conv_L);
	fftw_free(dft_freq_conv_R);

	return 1;
}
