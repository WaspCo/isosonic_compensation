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
#include "allocation.h"

/** perform a Catmull-Rom spline interpolation using 4 points
 * @param p1 (struct point)
 * @param p2 (struct point)
 * @param p3 (struct point)
 * @param p4 (struct point)
 * @param t distance between p2 & p3 (double)
 * @return result (double)
 */
double catmul_rom(
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
 * @param transfer_function to load into (TransferFunction**)
 * @param BUFFER_SIZE (const size_t)
 * @return 0 if ok, 0 else
 */
uint8_t craft_transfer_functions(
    FILE *isosonic_file,
    TransferFunction **transfer_functions,
    const size_t BUFFER_SIZE)
{

    size_t i = 0;
    size_t j = 0;

    char b1[2] = {'\0'};
    char b2[20] = {'\0'};

    const size_t NB_POINTS = 31;
    const size_t NB_CURVE = 90;

    TransferFunction **curve_raw = allocate_transfer_function(
        NB_POINTS,
        NB_CURVE);

    while (fread(b1, 1, 1, isosonic_file) == 1)
    {

        if ((j == NB_CURVE - 1) && (strcmp(&b1[0], ",") == 0))
        {

            if (i == NB_POINTS) // end of file
            {
                sscanf(b2, "%f", curve_raw[j]->level_at_1000hz);
                b2[0] = '\0';
                b1[0] = '\0';
            }

            else // end of line
            {
                sscanf(b2, "%f", curve_raw[j]->data[i]);
                fread(b1, 1, 1, isosonic_file);
                b2[0] = '\0';
                b1[0] = '\0';
                i++;
                j = 0;
            }
        }
        else if (strcmp(&b1[0], ",") == 0) // go to next line
        {

            if (i == NB_POINTS)
            {
                sscanf(b2, "%f", curve_raw[j]->level_at_1000hz);
            }
            else
            {
                sscanf(b2, "%f", curve_raw[j]->level_at_1000hz);
            }
            b2[0] = '\0';
            b1[0] = '\0';
            j++;
        }
        else // next column
        {

            strcat(b2, b1);
            b1[0] = '\0';
        }
    }

    // convert 1/3octave to linear && buffer scaling

    const size_t NB_NEW_POINTS = BUFFER_SIZE * 2;

    double q = 19980.0 / (NB_NEW_POINTS - 1);
    double pre = 0.0;
    double step = 0.0;

    size_t k = 0;
    
    size_t one = 0;
    size_t two = 0;

    for (j = 0; j < NB_CURVE; j++)
    {
        for (i = 0; i < NB_POINTS - 2; i++)
        {

            switch (i)
            {
            case 0:
            {
                one = 20;
                two = 25;
                break;
            }
            case 1:
            {
                one = 25;
                two = 31.5;
                break;
            }
            case 2:
            {
                one = 31.5;
                two = 40;
                break;
            }
            case 3:
            {
                one = 40;
                two = 50;
                break;
            }
            case 4:
            {
                one = 50;
                two = 63;
                break;
            }
            case 5:
            {
                one = 63;
                two = 80;
                break;
            }
            case 6:
            {
                one = 80;
                two = 100;
                break;
            }
            case 7:
            {
                one = 100;
                two = 125;
                break;
            }
            case 8:
            {
                one = 125;
                two = 160;
                break;
            }
            case 9:
            {
                one = 160;
                two = 200;
                break;
            }
            case 10:
            {
                one = 200;
                two = 250;
                break;
            }
            case 11:
            {
                one = 250;
                two = 315;
                break;
            }
            case 12:
            {
                one = 315;
                two = 400;
                break;
            }
            case 13:
            {
                one = 400;
                two = 500;
                break;
            }
            case 14:
            {
                one = 500;
                two = 630;
                break;
            }
            case 15:
            {
                one = 630;
                two = 800;
                break;
            }
            case 16:
            {
                one = 800;
                two = 1000;
                break;
            }
            case 17:
            {
                one = 1000; // this is the reference level
                two = 1250;
                break;
            }
            case 18:
            {
                one = 1250;
                two = 1600;
                break;
            }
            case 19:
            {
                one = 2000;
                two = 2500;
                break;
            }
            case 20:
            {
                one = 2500;
                two = 3150;
                break;
            }
            case 21:
            {
                one = 3150;
                two = 4000;
                break;
            }
            case 22:
            {
                one = 4000;
                two = 5000;
                break;
            }
            case 23:
            {
                one = 5000;
                two = 6300;
                break;
            }
            case 24:
            {
                one = 6300;
                two = 8000;
                break;
            }
            case 25:
            {
                one = 8000;
                two = 10000;
                break;
            }
            case 26:
            {
                one = 10000;
                two = 12500;
                break;
            }
            case 27:
            {
                one = 12500;
                two = 16000;
                break;
            }
            case 28:
            {
                one = 16000;
                two = 20000;
                break;
            }
            }

            step = 1.0 / ((two - one) / q);

            while (pre < 1)
            {
                transfer_functions[j]->data[k] = pow(10.0, linear(curve_raw[j]->data[i], curve_raw[j]->data[i + 1], pre) / 20.0);

                k += 1;

                pre += step;
            }

            pre = pre - 1.0; // spread the error
        }

        k = 0;

        pre = 0.;

        transfer_functions[j]->level_at_1000hz = curve_raw[j]->level_at_1000hz;
    }

    rewind(isosonic_file);

    deallocate_transfer_functions(curve_raw, NB_CURVE);

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

    // number of data points per curve
    const size_t NB_POINTS = 31;

    const size_t NB_ORIGINAL_CURVE = 9;
    const size_t NB_CURVE_FACTOR = 10;
    const size_t NB_NEW_CURVE = NB_ORIGINAL_CURVE * NB_CURVE_FACTOR;

    const size_t INDEX_1000HZ = 17;

    // maximum amount of correction applied to the signal
    const size_t MAX_CORRECTION_LEVEL = 40; // db SPL

    // level at which the signal was supposedly mixed
    const size_t ORIGINAL_LEVEL = 80; // db SPL

    const double c120[31] = {138.41, 134.15, 130.11, 126.38, 123.35, 120.65, 118.16, 116.17, 114.48, 113.03, 111.85, 110.97, 110.3, 109.83, 109.62, 109.5, 109.44, 110.01, 112.81, 114.25, 111.18, 108.48, 107.67, 109, 112.3, 117.23, 121.11, 120.23, 112.07, 110.83, 143.73};

    const double c100[31] = {128.41, 124.15, 120.11, 116.38, 113.35, 110.65, 108.16, 106.17, 104.48, 103.03, 101.85, 100.97, 100.3, 99.83, 99.62, 99.5, 99.44, 100.01, 102.81, 104.25, 101.18, 98.48, 97.67, 99, 102.3, 107.23, 111.11, 110.23, 102.07, 100.83, 133.73};

    const double c80[31] = {118.99, 114.23, 109.65, 105.34, 101.72, 98.36, 95.17, 92.48, 90.09, 87.82, 85.92, 84.31, 82.89, 81.68, 80.86, 80.17, 79.67, 80.01, 82.48, 83.74, 80.59, 77.88, 77.07, 78.31, 81.62, 86.81, 91.41, 91.74, 85.41, 84.67, 118.95};

    const double c60[31] = {109.51, 104.23, 99.08, 94.18, 89.96, 85.94, 82.05, 78.65, 75.56, 72.47, 69.86, 67.53, 65.39, 63.45, 62.05, 60.81, 59.89, 60.01, 62.15, 63.19, 59.96, 57.26, 56.42, 57.57, 60.89, 66.36, 71.66, 73.16, 68.63, 68.43, 104.92};

    const double c40[31] = {99.85, 93.94, 88.17, 82.63, 77.78, 73.08, 68.48, 64.37, 60.59, 56.7, 53.41, 50.4, 47.58, 44.98, 43.05, 41.34, 40.06, 40.01, 41.82, 42.51, 39.23, 36.51, 35.61, 36.65, 40.01, 45.83, 51.8, 54.28, 51.49, 51.96, 92.77};

    const double c20[31] = {89.58, 82.65, 75.98, 69.62, 64.02, 58.55, 53.19, 48.38, 43.94, 39.37, 35.51, 31.99, 28.69, 25.67, 23.43, 21.48, 20.1, 20.01, 21.46, 21.4, 18.15, 15.38, 14.26, 15.14, 18.63, 25.02, 31.52, 34.43, 33.04, 34.67, 84.18};

    const double c10[31] = {83.75, 75.76, 68.21, 61.14, 54.96, 49.01, 43.24, 38.13, 33.48, 28.77, 24.84, 21.33, 18.05, 15.14, 12.98, 11.18, 9.99, 10, 11.26, 10.43, 7.27, 4.45, 3.04, 3.8, 7.46, 14.35, 20.98, 23.43, 22.33, 25.17, 81.47};

    const double c0[31] = {76.55, 65.62, 55.12, 45.53, 37.63, 30.86, 25.02, 20.51, 16.65, 13.12, 10.09, 7.54, 5.11, 3.06, 1.48, 0.3, -0.3, -0.01, 1.03, -1.19, -4.11, -7.05, -9.03, -8.49, -4.48, 3.28, 9.83, 10.48, 8.38, 14.1, 79.65};

    struct point p1, p2, p3, p4;

    TransferFunction **curve_origin = allocate_transfer_function(
        NB_POINTS,
        NB_ORIGINAL_CURVE);

    // create output dir for curve if it does not exists
    struct stat st = {0};
    if (stat("curve/", &st) == -1)
        mkdir("curve/", 0700);

    ////////////////////////////////////////////////////////////////////////////
    // Remplissage des tableaux initiaux

    for (size_t i = 0; i < NB_ORIGINAL_CURVE; i++)
    {
        for (size_t j = 0; j < NB_POINTS; j++)
        {
            switch (i)
            {
            case 0:
            {
                curve_origin[i]->data[j] = 0.;
                break;
            }
            case 1:
            {
                curve_origin[i]->data[j] = c0[j];
                break;
            }
            case 2:
            {
                curve_origin[i]->data[j] = c10[j];
                break;
            }
            case 3:
            {
                curve_origin[i]->data[j] = c20[j];
                break;
            }
            case 4:
            {
                curve_origin[i]->data[j] = c40[j];
                break;
            }
            case 5:
            {
                curve_origin[i]->data[j] = c60[j];
                break;
            }
            case 6:
            {
                curve_origin[i]->data[j] = c80[j];
                break;
            }
            case 7:
            {
                curve_origin[i]->data[j] = c100[j];
                break;
            }
            case 8:
            {
                curve_origin[i]->data[j] = c120[j];
                break;
            }
            }
        }
    }

    FILE *curve_origin_file = NULL;

    curve_origin_file = fopen("curve/curve_origin.csv", "w");

    if (!curve_origin_file)
        return NULL;

    for (size_t i = 0; i < NB_POINTS; i++)
    {
        for (size_t j = 0; j < NB_ORIGINAL_CURVE; j++)
        {
            fprintf(curve_origin_file, "%f,", curve_origin[j]->data[i]);
        }
        fprintf(curve_origin_file, "\n");
    }
    fclose(curve_origin_file);

    ////////////////////////////////////////////////////////////////////////////
    // Interpolation Catmul-Rom

    TransferFunction **curve_itrp = allocate_transfer_function(
        NB_POINTS,
        NB_NEW_CURVE);

    size_t k = 0;
    float curve_step = 1. / NB_CURVE_FACTOR;

    for (size_t i = 0; i < NB_POINTS; i++)
    {
        for (size_t j = 0; j < NB_ORIGINAL_CURVE - 3; j++)
        {
            p1.x = j;
            p1.y = curve_origin[j]->data[i];
            p2.x = j + 1;
            p2.y = curve_origin[j + 1]->data[i];
            p3.x = j + 2;
            p3.y = curve_origin[j + 2]->data[i];
            p4.x = j + 3;
            p4.y = curve_origin[j + 3]->data[i];

            for (float t = 0; t <= 1; t += curve_step)
            {
                curve_itrp[k]->data[i] = catmul_rom(p1, p2, p3, p4, t);
                k++;
            }
        }
        k = 0;
    }

    FILE *curve_itrp_file = fopen("curve/curve_itrp.csv", "w");

    if (!curve_itrp_file)
    {
        return NULL;
    }

    for (size_t i = 0; i < NB_POINTS; i++)
    {
        for (size_t j = 0; j < NB_NEW_CURVE; j++)
        {
            fprintf(curve_itrp_file, "%lf,", curve_itrp[j]->data[i]);
        }
        fprintf(curve_itrp_file, "\n");
    }
    fclose(curve_itrp_file);

    deallocate_transfer_functions(curve_origin, NB_ORIGINAL_CURVE);

    ////////////////////////////////////////////////////////////////////////////
    // Interpolate with flat curve

    TransferFunction **curve_flat = allocate_transfer_function(
        NB_POINTS,
        NB_NEW_CURVE);

    for (size_t i = 0; i < NB_NEW_CURVE - 1; i++)
    {
        for (size_t j = 0; j < NB_POINTS; j++)
        {
            curve_flat[i]->data[j] = fmin(
                curve_itrp[i]->data[j],
                ORIGINAL_LEVEL);
        }
    }

    FILE *curve_flat_file = fopen("curve/curve_flat.csv", "w");

    if (!curve_flat_file)
        return NULL;

    for (size_t i = 0; i < NB_POINTS; i++)
    {
        for (size_t j = 0; j < NB_NEW_CURVE; j++)
        {
            fprintf(curve_flat_file, "%f,", curve_flat[j]->data[i]); 
        }
        fprintf(curve_flat_file, "\n");
    }
    fclose(curve_flat_file);

    deallocate_transfer_functions(curve_itrp, NB_NEW_CURVE);

    ////////////////////////////////////////////////////////////////////////////
    // Move curves down such that correction at 1000hz is 0db SPL
    // Also make sure that correction is never greater than 40db SPL
    // Because too much gain = signal degradation

    TransferFunction **curve_final = allocate_transfer_function(
        NB_POINTS,
        NB_NEW_CURVE);

    for (size_t i = 0; i < NB_NEW_CURVE; i++)
    {
        for (size_t j = 0; j < NB_POINTS; j++)
        {
            curve_final[i]->data[j] = curve_flat[i]->data[j] - curve_flat[i]->data[INDEX_1000HZ];

            curve_final[i]->level_at_1000hz = curve_flat[i]->data[INDEX_1000HZ];

            if (curve_final[i]->data[j] >= MAX_CORRECTION_LEVEL)
            {
                curve_final[i]->data[j] = MAX_CORRECTION_LEVEL;
            }
        }
    }

    FILE *curve_final_file = fopen("curve/curve_final.csv", "w");

    if (!curve_final_file)
        return NULL;

    for (size_t i = 0; i < NB_POINTS; i++)
    {
        for (size_t j = 0; j < NB_NEW_CURVE; j++)
        {
            fprintf(curve_final_file, "%f,", curve_final[j]->data[i]);
        }
        fprintf(curve_final_file, "\n");
    }

    for (size_t i = 0; i < NB_NEW_CURVE; i++)
    {
        fprintf(curve_final_file, "%f,", curve_final[i]->level_at_1000hz);
    }

    rewind(curve_final_file);

    deallocate_transfer_functions(curve_flat, NB_NEW_CURVE);
    deallocate_transfer_functions(curve_final, NB_NEW_CURVE);

    return curve_final_file;
}