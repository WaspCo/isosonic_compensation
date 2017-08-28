#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

// Programme de calcul des courbes interpolé avec flat à 80dB SPL.
// Création du tableau origin, puis calcul d'une interpolation catmulrom
// Le programme rabote ensuite les courbes aux valeurs <0 ou >80
// c_orig => c_itrp => c_fltr => c_flat => matrice.c

struct point {
  double x;        
  double y;
};

struct data {    // Contient les donnés finales + metadatas courbes
  double c[90][31];
  double mtdt[90];
};      // Les metadatas sont le niveau d'origine des courbes à 1kH.

double Interpolation(struct point p1, struct point p2, struct point p3, struct point p4, float t);
double Interpolation(struct point p1, struct point p2, struct point p3, struct point p4, float t)
{
    double c = 0;
    c = 0.5 *((2 * p2.y) +
  	         ((-p1.y + p3.y)*t) +
             ((2*p1.y - 5*p2.y + 4*p3.y - p4.y)*pow(t,2)) +
             ((-p1.y + 3*p2.y- 3*p3.y + p4.y)*pow(t,3)));
    return c;
}

int main(void)
{
      float pre=0.1;
/*
    printf("\n/////////////////////////////////////////////////////////////////////\n");
    printf("/////// Interpolation & Conformation des Courbes Isosoniques ///////\n");
    printf("/////////////////////////////////////////////////////////////////////\n\n");
    printf("Quelle est la précision d'interpolation par Spline de CatmulRom ?\n");
    scanf("%f\n", &pre);
    printf("Quelle est le niveau seuil supérieur ?\n");
    scanf("%d\n", &seuil_sup);
    printf("Quelle est le niveau seuil inférieur ?\n");
    scanf("%d\n", &seuil_inf);
    printf("Quelle est la précision d'interpolation par le seuil supérieur ? \n");
    scanf("%d\n", &pre_flat);
*/
      // c120 est générique, c'est c100 avec 10db en + en y. c120 est une extrémité nécessaire à catmultom pour calculer c80 >P> c100

      double c120[31] = {138.41,134.15,130.11,126.38,123.35,120.65,118.16,116.17,114.48,113.03,111.85,110.97,110.3,109.83,109.62,109.5,109.44,110.01,112.81,114.25,111.18,108.48,107.67,109,112.3,117.23,121.11,120.23,112.07,110.83,143.73};
    double c100[31] = {128.41,124.15,120.11,116.38,113.35,110.65,108.16,106.17,104.48,103.03,101.85,100.97,100.3,99.83,99.62,99.5,99.44,100.01,102.81,104.25,101.18,98.48,97.67,99,102.3,107.23,111.11,110.23,102.07,100.83,133.73};
    double c80[31] = {118.99,114.23,109.65,105.34,101.72,98.36,95.17,92.48,90.09,87.82,85.92,84.31,82.89,81.68,80.86,80.17,79.67,80.01,82.48,83.74,80.59,77.88,77.07,78.31,81.62,86.81,91.41,91.74,85.41,84.67,118.95};
    double c60[31] = {109.51,104.23,99.08,94.18,89.96,85.94,82.05,78.65,75.56,72.47,69.86,67.53,65.39,63.45,62.05,60.81,59.89,60.01,62.15,63.19,59.96,57.26,56.42,57.57,60.89,66.36,71.66,73.16,68.63,68.43,104.92};
    double c40[31] = {99.85,93.94,88.17,82.63,77.78,73.08,68.48,64.37,60.59,56.7,53.41,50.4,47.58,44.98,43.05,41.34,40.06,40.01,41.82,42.51,39.23,36.51,35.61,36.65,40.01,45.83,51.8,54.28,51.49,51.96,92.77};
    double c20[31] = {89.58,82.65,75.98,69.62,64.02,58.55,53.19,48.38,43.94,39.37,35.51,31.99,28.69,25.67,23.43,21.48,20.1,20.01,21.46,21.4,18.15,15.38,14.26,15.14,18.63,25.02,31.52,34.43,33.04,34.67,84.18};
    double c10[31] = {83.75,75.76,68.21,61.14,54.96,49.01,43.24,38.13,33.48,28.77,24.84,21.33,18.05,15.14,12.98,11.18,9.99,10,11.26,10.43,7.27,4.45,3.04,3.8,7.46,14.35,20.98,23.43,22.33,25.17,81.47};
    double c0[31] = {76.55,65.62,55.12,45.53,37.63,30.86,25.02,20.51,16.65,13.12,10.09,7.54,5.11,3.06,1.48,0.3,-0.3,-0.01,1.03,-1.19,-4.11,-7.05,-9.03,-8.49,-4.48,3.28,9.83,10.48,8.38,14.1,79.65};
    double zero[31] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    //double flat[31] = {80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80}; // Référence haute

    struct point p1;
    struct point p2;
    struct point p3;
    struct point p4;

    int i = 0;

    int nbc = 0;  // variable boucle de décalage de 4 points sur 6
    int nbci = 9/pre;
    float t=0;

    double origin[9][31];   // Courbes d'origine
    double itrp[nbci][31];    // Courbes après interpolation
    struct stat st = {0};

    if (stat("carve/", &st) == -1) {
    mkdir("carve/", 0700); }

////////////////////////////////////////////////////////////////////////////////
// Remplissage des tableaux initiaux

    for (nbc=0; nbc<9; nbc++)
    {
        for (i = 0 ; i < 31 ; i++)              //Boucle de remplissage tableau data d'origin
        {
            if (nbc==0){                         //Choix de la courbe
                origin[nbc][i] = zero[i];}
            else if (nbc==1){
                origin[nbc][i] = c0[i];}
            else if (nbc==2){
                origin[nbc][i] = c10[i];}
            else if (nbc==3){
                origin[nbc][i] = c20[i];}
            else if (nbc==4){
                origin[nbc][i] = c40[i];}
            else if (nbc==5){
                origin[nbc][i] = c60[i];}
            else if (nbc==6){
                origin[nbc][i] = c80[i];}
            else if (nbc==7){
                origin[nbc][i] = c100[i];}
            else if (nbc==8){
                origin[nbc][i] = c120[i];}
            //printf("i=%d, nbc=%d, y=%f\n", i, nbc, c_origin[nbc][i]);     // Controle visuel
        }
    }

    FILE *c_origin = fopen("carve/c_origin.csv", "w");
    for (i = 0; i < 31 ; i++)
    {
      for (nbc = 0; nbc < 9; nbc++)
      {
        fprintf(c_origin,"%f,", origin[nbc][i]);     // Export .csv
      }
      fprintf(c_origin,"\n");
    }
    fclose(c_origin);

////////////////////////////////////////////////////////////////////////////////
// Interpolation Catmul-Rom

    for (i=0; i<31; i++)             // Début de la boucle calcul interpolation ...
    {
      nbci = 0;
      for (nbc=0; nbc<6; nbc++)
      {
        p1.x = nbc ;               // Initialisation des points d'origine
        p1.y = origin[nbc][i];
        p2.x = nbc+1 ;
        p2.y = origin[nbc+1][i];
        p3.x = nbc+2 ;
        p3.y = origin[nbc+2][i];
        p4.x = nbc+3 ;
        p4.y = origin[nbc+3][i];

        for (t=0; t<=1; t=t+pre) {     // On balaye pre <= t <= 1 pour obtenir les points intermédiaire

          itrp[nbci][i] = Interpolation(p1, p2, p3, p4, t);     // Appel de l'inter* catmulrom, retourne y
          //printf("i=%d, nbci=%d, y=%f\n", i, nbci, c_itrp[nbci][i]);     // Controle visuel
          nbci++;  }
      }
    }

    FILE *c_itrp = fopen("carve/c_itrp.csv", "w");
    for (i = 0; i < 31 ; i++)
    {
      for (nbci = 0; nbci < 90; nbci++)
      {
        fprintf(c_itrp,"%f,", itrp[nbci][i]);     // Export .csv
      }
      fprintf(c_itrp,"\n");
    }
    fclose(c_itrp);

////////////////////////////////////////////////////////////////////////////////
// Interpolation avec la courbe flat

    double f = 1, pre_flat = 1;      // Précision d'interpolation avec la flat, répétition de 0.1 à 1
    int nbcf=0;
    double flat[90][31];

    nbcf=0;
    for (nbci = 0; nbci < 90; nbci++)     // Boucle de nb courbes filtré
    {
      while ( fmin(80,((f * itrp[nbci][17]) + ((1-f) * 80))) <= fmin(80,itrp[nbci+1][17]) && f > 0 )
      {
        for (i = 0; i < 31 ; i++)
        {
          flat[nbcf][i] = fmin(80,((f * itrp[nbci][i]) + ((1-f) * 80)));
        }
        nbcf++;
        f = f - pre_flat;
      }
    f = 1;
    }

    FILE *c_flat = fopen("carve/c_flat.csv", "w");
    for (i = 0; i < 31 ; i++)
    {
      for (nbcf = 0; nbcf < 90; nbcf++)
      {
        fprintf(c_flat,"%f,", flat[nbcf][i]);     // Export .csv
      }
      fprintf(c_flat,"\n");
    }
    fclose(c_flat);

////////////////////////////////////////////////////////////////////////////////
// Mise en forme finale

    struct data matrice;

    for (i=0; i < 31; i++)
    {
      for (nbcf=0; nbcf < 90; nbcf++)
      {
        matrice.c[nbcf][i] = flat[nbcf][i] - flat[nbcf][17];
        matrice.mtdt[nbcf] = flat[nbcf][17];    // Réf de courbe à 1kH
        //printf("i=%d, nbcf=%d, y=%f, réf=%f\n", i, nbcf, matrice.c[nbcf][i], matrice.mtdt[nbcf]);     // Controle visuel
      }
    }

////////////////////////////////////////////////////////////////////////////////
// Filtrage, on n'amplifie pas à plus de 40dB (trop de gain = dégradation signal)

    for (i = 0; i < 31 ; i++)
    {
        for (nbcf = 0; nbcf < 90; nbcf++)
        {
            if (matrice.c[nbcf][i] >= 40) { matrice.c[nbcf][i] = 40; }
        }
    }

////////////////////////////////////////////////////////////////////////////////
// Exportation des courbes

    FILE *c_matrice = fopen("carve/c_matrice.csv", "w");
    for (i = 0; i < 31 ; i++)
    {
      for (nbcf = 0; nbcf < 90; nbcf++)
      {
        fprintf(c_matrice,"%f,", matrice.c[nbcf][i]);     // Export .csv
      }
      fprintf(c_matrice,"\n");
    }

    //Les metedatas de chaque courbe sont à la 91ème colonne
    for (nbcf = 0; nbcf < 90; nbcf++)
    {
      fprintf(c_matrice,"%f,", matrice.mtdt[nbcf]);
    }
    fclose(c_matrice);


return 0;
}
