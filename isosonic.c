/* @file isosonic.c
 * Load and process isosonic curves. Create set of transfer functions from them
 *
 * Source:
 * - https://en.wikipedia.org/wiki/Equal-loudness_contour
 * Auteur:
 * Victor Deleau
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "isosonic.h"
#include "loudness.h"

// Programme de calcul des courbes interpolé avec flat à 80dB SPL.
// Création du tableau origin, puis calcul d'une interpolation catmulrom
// Le programme rabote ensuite les courbes aux valeurs <0 ou >80
// c_orig => curve_itrp => c_fltr => curve_flat => curve.c

/** perform a Catmull-Rom spline interpolation using 4 points
 * @param p1 (struct point)
 * @param p2 (struct point)
 * @param p3 (struct point)
 * @param p4 (struct point)
 * @param t distance between p2 & p3 (double)
 * @return result (double)
 */
double Interpolation(
    struct point p1,
    struct point p2,
    struct point p3,
    struct point p4,
    double t)
{
    return 0.5 * ((2 * p2.y) +
               ((-p1.y + p3.y) * t) +
               ((2 * p1.y - 5 * p2.y + 4 * p3.y - p4.y) * pow(t, 2)) +
               ((-p1.y + 3 * p2.y - 3 * p3.y + p4.y) * pow(t, 3)));
}

/** linear interpolation with t parameter
 * @param first value (float)
 * @param second value (float)
 * @param t between 0 & 1 (double)
 * @return result (float)
 */
float linear(float x, float y, double t)
{
    return ((1 - t) * x) + (t * y);
}

/** Parse the isophonic curve file and craft the transfer function
 * @param isosonic_file (FILE*)
 * @param transfer_function to load into (struct transfer_function)
 * @param buffer_size (unsigned_int)
 * @return 0 if ok, 0 else
 */
uint8_t craft_transfer_function(
    FILE *isosonic_file,
    struct transfer_function *transfer_function,
    unsigned int *buffer_size)
{
    int i = 0, j = 0;
    char b1[2] = {'\0'};
    char b2[20] = {'\0'};

    struct curve curve_raw;

    // parse csv file
    while (fread(b1, 1, 1, isosonic_file) == 1)
    {

        if ((j == 89) && (strcmp(&b1[0], ",") == 0))
        {

            if (i == 31)
            { // end of file
                sscanf(b2, "%f", curve_raw.metadata[j]);
                b2[0] = '\0';
                b1[0] = '\0';
            }

            else
            { // end of line
                sscanf(b2, "%f", curve_raw.data[i][j]);
                fread(b1, 1, 1, isosonic_file); // eating the back to line
                b2[0] = '\0';
                b1[0] = '\0';
                i++;
                j = 0;
            }
        }
        else if (strcmp(&b1[0], ",") == 0)
        { // go to next line

            if (i == 31)
            { // metadata save
                sscanf(b2, "%f", curve_raw.metadata[j]);
            }
            else
            { // data save
                sscanf(b2, "%f", curve_raw.data[i][j]);
            }
            b2[0] = '\0';
            b1[0] = '\0';
            j++;
        }
        else
        { // next column

            strcat(b2, b1);
            b1[0] = '\0';
        }
    }

    // convert 1/3octave to linear && buffer scaling
    double q = 19980.0 / ((*buffer_size * 2) - 1);
    double pre = 0.0;
    double n = 0.0;
    int k = 0, one = 0, two = 0;

    for (int j = 0; j < 90; j++)
    {
        for (int i = 0; i < 29; i++)
        {

            if (i == 0)
            {
                one = 20;
                two = 25;
            }
            else if (i == 1)
            {
                one = 25;
                two = 31.5;
            }
            else if (i == 2)
            {
                one = 31.5;
                two = 40;
            }
            else if (i == 3)
            {
                one = 40;
                two = 50;
            }
            else if (i == 4)
            {
                one = 50;
                two = 63;
            }
            else if (i == 5)
            {
                one = 63;
                two = 80;
            }
            else if (i == 6)
            {
                one = 80;
                two = 100;
            }
            else if (i == 7)
            {
                one = 100;
                two = 125;
            }
            else if (i == 8)
            {
                one = 125;
                two = 160;
            }
            else if (i == 9)
            {
                one = 160;
                two = 200;
            }
            else if (i == 10)
            {
                one = 200;
                two = 250;
            }
            else if (i == 11)
            {
                one = 250;
                two = 315;
            }
            else if (i == 12)
            {
                one = 315;
                two = 400;
            }
            else if (i == 13)
            {
                one = 400;
                two = 500;
            }
            else if (i == 14)
            {
                one = 500;
                two = 630;
            }
            else if (i == 15)
            {
                one = 630;
                two = 800;
            }
            else if (i == 16)
            {
                one = 800;
                two = 1000;
            }
            else if (i == 17)
            {
                one = 1000;
                two = 1250;
            }
            else if (i == 18)
            {
                one = 1250;
                two = 1600;
            }
            else if (i == 19)
            {
                one = 2000;
                two = 2500;
            }
            else if (i == 20)
            {
                one = 2500;
                two = 3150;
            }
            else if (i == 21)
            {
                one = 3150;
                two = 4000;
            }
            else if (i == 22)
            {
                one = 4000;
                two = 5000;
            }
            else if (i == 23)
            {
                one = 5000;
                two = 6300;
            }
            else if (i == 24)
            {
                one = 6300;
                two = 8000;
            }
            else if (i == 25)
            {
                one = 8000;
                two = 10000;
            }
            else if (i == 26)
            {
                one = 10000;
                two = 12500;
            }
            else if (i == 27)
            {
                one = 12500;
                two = 16000;
            }
            else if (i == 28)
            {
                one = 16000;
                two = 20000;
            }

            n = 1.0 / ((two - one) / q);

            while (pre < 1)
            {
                transfer_function->data[k][j] = pow(10.0, linear(curve_raw.data[i][j], curve_raw.data[i + 1][j], pre) / 20.0);
                k++;
                pre += n;
            }
            pre = pre - 1.0; // spread error
        }

        k = 0;
        pre = 0.;
    }

    for (int j = 0; j < 90; j++)
    {
        transfer_function->metadata[j] = curve_raw.metadata[j];
    }

    return 0;
}

/** Create the transfer function from the isosonic curve data
 * There are several steps:
 * 1 - load the different curves according to their listening level
 * 2 - use a Catmull-Rom interpolation to compute in-between curves
 * 3 - use a linear interpolation with 80dbSPL listening level
 *     such that no correction is applied at 80dbSPL and higher
 *     80dbSPL being the level at which the audio was supposedly mixed
 * 4 - subtract each curve its level at 1kHz to obtain transfer functions
 * 5 - limit the correction level to 40dbSPL as no to add too much distortion
 * @return a pointer to the transfer function file (FILE*)
 */
FILE *process_isosonic_curve(void)
{
    float pre = 0.1;

    const double c120[31] = {138.41, 134.15, 130.11, 126.38, 123.35, 120.65, 118.16, 116.17, 114.48, 113.03, 111.85, 110.97, 110.3, 109.83, 109.62, 109.5, 109.44, 110.01, 112.81, 114.25, 111.18, 108.48, 107.67, 109, 112.3, 117.23, 121.11, 120.23, 112.07, 110.83, 143.73};

    const double c100[31] = {128.41, 124.15, 120.11, 116.38, 113.35, 110.65, 108.16, 106.17, 104.48, 103.03, 101.85, 100.97, 100.3, 99.83, 99.62, 99.5, 99.44, 100.01, 102.81, 104.25, 101.18, 98.48, 97.67, 99, 102.3, 107.23, 111.11, 110.23, 102.07, 100.83, 133.73};

    const double c80[31] = {118.99, 114.23, 109.65, 105.34, 101.72, 98.36, 95.17, 92.48, 90.09, 87.82, 85.92, 84.31, 82.89, 81.68, 80.86, 80.17, 79.67, 80.01, 82.48, 83.74, 80.59, 77.88, 77.07, 78.31, 81.62, 86.81, 91.41, 91.74, 85.41, 84.67, 118.95};

    const double c60[31] = {109.51, 104.23, 99.08, 94.18, 89.96, 85.94, 82.05, 78.65, 75.56, 72.47, 69.86, 67.53, 65.39, 63.45, 62.05, 60.81, 59.89, 60.01, 62.15, 63.19, 59.96, 57.26, 56.42, 57.57, 60.89, 66.36, 71.66, 73.16, 68.63, 68.43, 104.92};

    const double c40[31] = {99.85, 93.94, 88.17, 82.63, 77.78, 73.08, 68.48, 64.37, 60.59, 56.7, 53.41, 50.4, 47.58, 44.98, 43.05, 41.34, 40.06, 40.01, 41.82, 42.51, 39.23, 36.51, 35.61, 36.65, 40.01, 45.83, 51.8, 54.28, 51.49, 51.96, 92.77};

    const double c20[31] = {89.58, 82.65, 75.98, 69.62, 64.02, 58.55, 53.19, 48.38, 43.94, 39.37, 35.51, 31.99, 28.69, 25.67, 23.43, 21.48, 20.1, 20.01, 21.46, 21.4, 18.15, 15.38, 14.26, 15.14, 18.63, 25.02, 31.52, 34.43, 33.04, 34.67, 84.18};

    const double c10[31] = {83.75, 75.76, 68.21, 61.14, 54.96, 49.01, 43.24, 38.13, 33.48, 28.77, 24.84, 21.33, 18.05, 15.14, 12.98, 11.18, 9.99, 10, 11.26, 10.43, 7.27, 4.45, 3.04, 3.8, 7.46, 14.35, 20.98, 23.43, 22.33, 25.17, 81.47};

    const double c0[31] = {76.55, 65.62, 55.12, 45.53, 37.63, 30.86, 25.02, 20.51, 16.65, 13.12, 10.09, 7.54, 5.11, 3.06, 1.48, 0.3, -0.3, -0.01, 1.03, -1.19, -4.11, -7.05, -9.03, -8.49, -4.48, 3.28, 9.83, 10.48, 8.38, 14.1, 79.65};
    const double zero[31] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    struct point p1;
    struct point p2;
    struct point p3;
    struct point p4;

    int i = 0;

    int nbc = 0; // variable boucle de décalage de 4 points sur 6
    int nbci = 9 / pre;
    float t = 0;

    double origin[9][31];  // Courbes d'origine
    double itrp[nbci][31]; // Courbes après interpolation

    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 31; j++)
        {
            origin[i][j] = 0.;
        }
    }

    for (int i = 0; i < nbci; i++)
    {
        for (int j = 0; j < 31; j++)
        {
            itrp[i][j] = 0.;
        }
    }

    struct stat st = {0};

    if (stat("curve/", &st) == -1) mkdir("curve/", 0700);


    ////////////////////////////////////////////////////////////////////////////
    // Remplissage des tableaux initiaux

    for (nbc = 0; nbc < 9; nbc++)
    {
        for (i = 0; i < 31; i++) //Boucle de remplissage tableau data d'origin
        {
            if (nbc == 0)
            { //Choix de la courbe
                origin[nbc][i] = zero[i];
            }
            else if (nbc == 1)
            {
                origin[nbc][i] = c0[i];
            }
            else if (nbc == 2)
            {
                origin[nbc][i] = c10[i];
            }
            else if (nbc == 3)
            {
                origin[nbc][i] = c20[i];
            }
            else if (nbc == 4)
            {
                origin[nbc][i] = c40[i];
            }
            else if (nbc == 5)
            {
                origin[nbc][i] = c60[i];
            }
            else if (nbc == 6)
            {
                origin[nbc][i] = c80[i];
            }
            else if (nbc == 7)
            {
                origin[nbc][i] = c100[i];
            }
            else if (nbc == 8)
            {
                origin[nbc][i] = c120[i];
            }
            //printf("i=%d, nbc=%d, y=%lf\n", i, nbc, curve_origin[nbc][i]);     // Controle visuel
        }
    }

    FILE *curve_origin = NULL;
    curve_origin = fopen("curve/curve_origin.csv", "w");

    if (!curve_origin) return NULL;

    for (i = 0; i < 31; i++)
    {
        for (nbc = 0; nbc < 9; nbc++)
        {
            fprintf(curve_origin, "%f,", origin[nbc][i]); // Export .csv
        }
        fprintf(curve_origin, "\n");
    }
    fclose(curve_origin);

    ////////////////////////////////////////////////////////////////////////////
    // Interpolation Catmul-Rom

    for (i = 0; i < 31; i++) // Début de la boucle calcul interpolation ...
    {
        nbci = 0;
        for (nbc = 0; nbc < 6; nbc++)
        {
            p1.x = nbc; // Initialisation des points d'origine
            p1.y = origin[nbc][i];
            p2.x = nbc + 1;
            p2.y = origin[nbc + 1][i];
            p3.x = nbc + 2;
            p3.y = origin[nbc + 2][i];
            p4.x = nbc + 3;
            p4.y = origin[nbc + 3][i];

            for (t = 0; t <= 1; t = t + pre)
            { // On balaye pre <= t <= 1 pour obtenir les points intermédiaire

                itrp[nbci][i] = Interpolation(p1, p2, p3, p4, t); // Appel de l'inter* catmulrom, retourne y
                //printf("i=%d, nbci=%d, y=%lf\n", i, nbci, curve_itrp[nbci][i]);     // Controle visuel
                nbci++;
            }
        }
    }

    FILE *curve_itrp = NULL;
    curve_itrp = fopen("curve/curve_itrp.csv", "w");

    if (!curve_origin) return NULL;

    for (i = 0; i < 31; i++)
    {
        for (nbci = 0; nbci < 90; nbci++)
        {
            fprintf(curve_itrp, "%lf,", itrp[nbci][i]); // Export .csv
        }
        fprintf(curve_itrp, "\n");
    }
    fclose(curve_itrp);

    ////////////////////////////////////////////////////////////////////////////
    // Interpolation avec la courbe flat

    double f = 1, pre_flat = 1; // Précision d'interpolation avec la flat, répétition de 0.1 à 1
    int nbcf = 0;

    double flat[90][31];

    for (int i = 0; i < 90; i++)
    {
        for (int j = 0; j < 31; j++)
        {
            flat[i][j] = 0.;
        }
    }

    nbcf = 0;
    for (nbci = 0; nbci < 90; nbci++) // Boucle de nb courbes filtré
    {
        while (fmin(80, ((f * itrp[nbci][17]) + ((1 - f) * 80))) <= fmin(80, itrp[nbci + 1][17]) && f > 0)
        {
            for (i = 0; i < 31; i++)
            {
                flat[nbcf][i] = fmin(80, ((f * itrp[nbci][i]) + ((1 - f) * 80)));
            }
            nbcf++;
            f = f - pre_flat;
        }
        f = 1;
    }

    FILE *curve_flat = NULL;
    curve_flat = fopen("curve/curve_flat.csv", "w");

    if (!curve_origin) return NULL;

    for (i = 0; i < 31; i++)
    {
        for (nbcf = 0; nbcf < 90; nbcf++)
        {
            fprintf(curve_flat, "%f,", flat[nbcf][i]); // Export .csv
        }
        fprintf(curve_flat, "\n");
    }
    fclose(curve_flat);

    ////////////////////////////////////////////////////////////////////////////
    // Mise en forme finale

    struct curve curve_final;

    for (int i = 0; i < 90; i++)
    {
        for (int j = 0; j < 31; j++)
        {
            curve_final.data[i][j] = 0.;
        }
        curve_final.metadata[i] = 0.;
    }

    for (i = 0; i < 31; i++)
    {
        for (nbcf = 0; nbcf < 90; nbcf++)
        {
            curve_final.data[nbcf][i] = flat[nbcf][i] - flat[nbcf][17];
            curve_final.metadata[nbcf] = flat[nbcf][17]; // Réf de courbe à 1kH
            //printf("i=%d, nbcf=%d, y=%lf, réf=%lf\n", i, nbcf, curve.data[nbcf][i], curve.metadata[nbcf]);     // Controle visuel
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // on n'amplifie pas à plus de 40dB (trop de gain = dégradation signal)

    for (i = 0; i < 31; i++)
    {
        for (nbcf = 0; nbcf < 90; nbcf++)
        {
            if (curve_final.data[nbcf][i] >= 40)
            {
                curve_final.data[nbcf][i] = 40;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Exportation des courbes

    FILE *transfer_function = NULL;
    transfer_function = fopen("curve/transfer_function.csv", "w");

    if (!curve_origin) return NULL;

    for (i = 0; i < 31; i++)
    {
        for (nbcf = 0; nbcf < 90; nbcf++)
        {
            fprintf(transfer_function, "%f,", curve_final.data[nbcf][i]); // Export .csv
        }
        fprintf(transfer_function, "\n");
    }

    //Les metedatas de chaque courbe sont à la 91ème colonne
    for (nbcf = 0; nbcf < 90; nbcf++)
    {
        fprintf(transfer_function, "%f,", curve_final.metadata[nbcf]);
    }

    return transfer_function;
}
