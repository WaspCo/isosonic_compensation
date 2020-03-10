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

#include "loudness.h"
#include "wave.h"

/** linear interpolation with t parameter
 * @param first value (float)
 * @param second value (float)
 * @param precision between 0 & 1 (float)
 * @return result (float)
 */
float linear(float *in, float *out, double *precision){
  float interpol = 0;
  interpol = ( ((1-(*precision))*(*in)) + ((*precision)*(*out)) );
  return interpol;
}

/** Load the isophonic curve file located in the root directory
 * @param ptr to isophonic curves file
 * @param c_matrice, isophonic curves destination
 * @param c_temp, isophonic curves before processing
 * @param buffer_size
 * @return integer
 */
int load_isophonic(FILE* iso_file, struct data *c_matrice, struct data *c_temp, unsigned int *buffer_size){

  int i = 0, j = 0;
  char b1[2] = {'\0'};
  char b2[20] = {'\0'};

  // parse csv file
  while ( fread(b1, 1, 1, iso_file) == 1 ){

    if((j == 89) && (strcmp( &b1[0], ",") == 0)) {

      if(i == 31){  // end of file
        sscanf(b2, "%f", &(c_temp->mtdt[j]));
        b2[0] = '\0';
        b1[0] = '\0';
      }

      else {  // end of line
        sscanf(b2, "%f", &(c_temp->c[i][j]));
        fread(b1, 1, 1, iso_file);  // eating the back to line
        b2[0] = '\0';
        b1[0] = '\0';
        i++;
        j = 0;
      }

    } else if(strcmp( &b1[0], ",") == 0) { // go to next line

      if(i==31){ sscanf(b2, "%f", &(c_temp->mtdt[j]));}  // metadata save
      else { sscanf(b2, "%f", &(c_temp->c[i][j]));}  // data save
      b2[0] = '\0';
      b1[0] = '\0';
      j++;

    } else { // next column

      strcat(b2, b1);
      b1[0] = '\0';
    }
  }

  // convert 1/3octave to linear && buffer scaling
  double q = 19980.0 / ((*buffer_size*2)-1);
  double pre = 0.0;
  double n = 0.0;
  int k = 0, one = 0, two = 0;

  for(int j=0; j<90; j++){
    for (int i=0; i<29; i++){

      if(i == 0){ one = 20; two = 25; }
      else if(i == 1){ one = 25; two = 31.5; }
      else if(i == 2){ one = 31.5; two = 40; }
      else if(i == 3){ one = 40; two = 50; }
      else if(i == 4){ one = 50; two = 63; }
      else if(i == 5){ one = 63; two = 80; }
      else if(i == 6){ one = 80; two = 100; }
      else if(i == 7){ one = 100; two = 125; }
      else if(i == 8){ one = 125; two = 160; }
      else if(i == 9){ one = 160; two = 200; }
      else if(i == 10){ one = 200; two = 250; }
      else if(i == 11){ one = 250; two = 315; }
      else if(i == 12){ one = 315; two = 400; }
      else if(i == 13){ one = 400; two = 500; }
      else if(i == 14){ one = 500; two = 630; }
      else if(i == 15){ one = 630; two = 800; }
      else if(i == 16){ one = 800; two = 1000; }
      else if(i == 17){ one = 1000; two = 1250; }
      else if(i == 18){ one = 1250; two = 1600; }
      else if(i == 19){ one = 2000; two = 2500; }
      else if(i == 20){ one = 2500; two = 3150; }
      else if(i == 21){ one = 3150; two = 4000; }
      else if(i == 22){ one = 4000; two = 5000; }
      else if(i == 23){ one = 5000; two = 6300; }
      else if(i == 24){ one = 6300; two = 8000; }
      else if(i == 25){ one = 8000; two = 10000; }
      else if(i == 26){ one = 10000; two = 12500; }
      else if(i == 27){ one = 12500; two = 16000; }
      else if(i == 28){ one = 16000; two = 20000; }

      n = 1.0 / (( two - one ) / q);

      while (pre < 1){
        c_matrice->c[k][j] = pow(10.0 , linear(&c_temp->c[i][j], &c_temp->c[i+1][j], &pre) / 20.0) ;
        k++;
        pre += n;
      }
      pre = pre - 1.0; // error spreading
    }

    k = 0;
    pre = 0;
  }

  for(int j = 0; j<90; j++) { c_matrice->mtdt[j] = c_temp->mtdt[j]; }


    //Pour tester la bonne lecture du fichier
    /*FILE * test = fopen("test.csv", "w");
    for (i = 0; i < (*buffer_size*2) ; i++)
    {
      for (j = 0; j < 90; j++)
      {
        fprintf(test, "%f,", c_matrice->c[i][j]);     // Export .csv
      }
      fprintf(test,"\n");
    }

    for (j = 0; j < 90; j++)
    {
      fprintf(test,"%f,", c_matrice->mtdt[j]);
    }
    fclose(test);*/

    /* Sans calibration, nous assumerons qu'un niveau d'écoute de 80dB SPL
     * correspond à un niveau interne de -18dBSPL*/

return 1;
}



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
 * @param c_matrice, isophonic curves
 * @param listening level
 * @return integer
 */
 int loudness(Header *header, unsigned int *buffer_size, fftw_complex dft_freq_L[], fftw_complex dft_freq_R[],
     fftw_complex dft_freq_conv_L[], fftw_complex dft_freq_conv_R[], struct data *c_matrice, int level){

  int last = 55 - 1;
  int first = 0;
  int middle = (first + last)/2;
  int curve2apply = 0;

  while(first <= last){ // binary search of the right curve to apply

    if(level > (c_matrice->mtdt[middle] + 0.5)) { first = middle + 1;}
    else if(level == (c_matrice->mtdt[middle] + 0.5)) { break; }
    else { last = middle - 1; }

    middle = (first + last) / 2;
  }

  curve2apply = middle + 1;

  int i = 0;
  float to_apply = 0;

  // apply correction to samples
  for ( i = 0; i < (*buffer_size/2 + 1); i++ ){

    // correction in dB SPL to linear scale
    to_apply = c_matrice->c[i][curve2apply] * 8;
    to_apply = pow(10,c_matrice->c[i][curve2apply]/20);

    // apply to left channel
    dft_freq_conv_L[i][0] = dft_freq_L[i][0] * to_apply;
    dft_freq_conv_R[i][0] = dft_freq_R[i][0] * to_apply;

    // apply to right channel
    dft_freq_conv_L[i][1] = dft_freq_L[i][1] * to_apply;
    dft_freq_conv_R[i][1] = dft_freq_R[i][1] * to_apply;
  }

  return 1;
}
