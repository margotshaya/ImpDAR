/*
 * mig_cython.c
 * Copyright (C) 2019 dlilien <dlilien@berens>
 *
 * Distributed under terms of the MIT license.
 */

#include "mig_cython.h"
#include <math.h>
#include <stdio.h>
#include <time.h> 
#include <signal.h>
#include <stdlib.h>
#include <Python.h>

/*  Migrate data into migdata */
void mig_kirch_loop (double * migdata, int tnum, int snum, double * dist, double * zs, double * zs2, double * tt_sec, double vel, double * gradD, double max_travel_time, int nearfield){
    int i, j, k, l;
    double min, m;
    double costheta;
    double integral;
    double r;
    int Didx;
    int lmin_prev, rmin_prev;
    clock_t iter_start, this_time;
    float time_remaining, el_t;
    setbuf(stdout, NULL);
    PySys_WriteStdout("Beginning migration");

    iter_start = clock();
    /* You can flip the loop order, but I think this is marginally faster
     * based on c row-major ordering. Could be wrong though... */

    /* Loop through every point in the image (trace and sample) 
     * to look for a hyperbola propagating away from that point */
    for(j=0;j<tnum;j++){
        if (j % 100 == 0){
            if (j > 0){
                PySys_WriteStdout("trace %d", j);
            }
        }else if (j % 10 == 0){
            PySys_WriteStdout(".");
        }
        for(i=0;i<snum;i++){
            integral = 0.0;
            rmin_prev = i;
            lmin_prev = i;

            /* Now look for the hyperbola propagating from this point */

            /* Do left and right separately so we know when to break
            * the break lets us skip a lot of iterations for long profiles */
            for(k=j + 1;k<tnum;k++){
                /* r is the distance from the surface to 
                 * this point along the hyperbola */
                r = sqrt(pow(dist[k] - dist[j], 2.) + zs2[i]);
                /* We do not get closer as we go, so we can break the loop
                 * if we are far away */
                if(2. * r / vel > max_travel_time){
                     break;
                }
                min = max_travel_time;
                /* The new min should not be smaller than the previous rmin
                * since otherwise the hyperbola would be inverted (smiles) */
                for(l=rmin_prev;l<snum;l++){
                    m = fabs(tt_sec[l] - 2. * r / vel);
                    if(m < min){
                        min = m;
                        Didx = l;
                    }
                }
                rmin_prev = Didx;
                costheta = zs[i] / r;
                integral += gradD[Didx * tnum + k] * costheta / vel;
            }

            for(k=j;k>=0;k--){
                r = sqrt(pow(dist[k] - dist[j], 2.) + zs2[i]);
                if(2. * r / vel > max_travel_time){
                     break;
                }
                min = max_travel_time;
                for(l=lmin_prev;l<snum;l++){
                    m = fabs(tt_sec[l] - 2. * r / vel);
                    if(m < min){
                        min = m;
                        Didx = l;
                    }
                }
                lmin_prev = Didx;
                costheta = zs[i] / r;
                integral += gradD[Didx * tnum + k] * costheta / vel;
            }

            migdata[i * tnum + j] = integral / (2*M_PI);
        }
        if (j % 100 == 0){
            if (j > 0){
                this_time = clock();
                el_t = (this_time - iter_start) / CLOCKS_PER_SEC;
                time_remaining = (tnum - j) * el_t / j;
                PySys_WriteStdout("\nEst. time remaining: %4.2f sec", time_remaining);
            }
        }
    }
    PySys_WriteStdout("\n");
}
