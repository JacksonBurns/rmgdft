/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 

/*
 *
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <complex.h>

#include "main.h"
#include "init_var.h"
#include "LCR.h"
#include "pmo.h"


void get_dos (STATE * states)
{
    int iprobe, idx_e;
    int iene, st1;
    complex double *tot, *tott, *g;
    complex double *sigma;
    rmg_double_t *temp_matrix_tri, *temp_matrix, *matrix_product;
    rmg_double_t de, emin, emax;

    complex double *green_C, *ch0, *ch01, *ch10;
    complex double ene, ctem;
    int nC;
    int i, j, *sigma_idx, idx_C;
    char fcd_n = 'N', fcd_c = 'C';
    FILE *file;
    double *H10, *S10;
    double one = 1.0, zero = 0.0;
    int ione = 1;

    int ntot, ndim;
    int  xoff, yoff, zoff;
    rmg_double_t *Green_store, *rho_energy, *rho_energy2;
    int root_pe, idx, ix, iy, iz;

    int E_POINTS, kpoint[3];
    double E_imag, KT;
    int FPYZ = get_FPY0_GRID() * get_FPZ0_GRID();
    int nx1, nx2, ny1, ny2, nz1, nz2; 
    int *desca, *descb, numst, numstC;

    read_cond_input (&emin, &emax, &E_POINTS, &E_imag, &KT, kpoint);
    de = (emax - emin) / (E_POINTS - 1);



    /* store the imaginary part of Green function Gc on each processor
     * then calculate the energy dependent charge density 
     * and stored in rho_energy after average in the yz plane
     */

    nC = ct.num_states;

/*========================== Reading Matrices =======================*/

    ntot = 0;
    ndim = 0;
    for (i = 0; i < ct.num_blocks; i++)
    {
        ntot += pmo.mxllda_cond[i] * pmo.mxlocc_cond[i];
        ndim += ct.block_dim[i];
    }
    for (i = 1; i < ct.num_blocks; i++)
    {
        ntot += pmo.mxllda_cond[i-1] * pmo.mxlocc_cond[i];
    }
    if (ndim != ct.num_states)
    {
        printf (" %d %d ndim not equal to nC in get_cond_frommatrix\n", ndim, ct.num_states);
        exit (0);
    }


    my_malloc_init( lcr[0].Htri, ntot, rmg_double_t );
    my_malloc_init( lcr[0].Stri, ntot, rmg_double_t );

    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx = pmo.mxllda_lead[iprobe-1] * pmo.mxlocc_lead[iprobe-1];
        my_malloc_init( lcr[iprobe].H00, idx, rmg_double_t );
        my_malloc_init( lcr[iprobe].S00, idx, rmg_double_t );
        my_malloc_init( lcr[iprobe].H01, idx, rmg_double_t );
        my_malloc_init( lcr[iprobe].S01, idx, rmg_double_t );
    }

    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        i = cei.probe_in_block[iprobe - 1];
        idx = pmo.mxllda_cond[i] * pmo.mxlocc_lead[iprobe-1];
        my_malloc_init( lcr[iprobe].HCL, idx, rmg_double_t );
        my_malloc_init( lcr[iprobe].SCL, idx, rmg_double_t );
    }


    read_matrix_pp();


/*======================== Allocate memory for sigma =================*/

    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx_C = cei.probe_in_block[iprobe - 1];  /* block index */
        idx = max(idx, pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C]);
    }
    my_malloc_init( sigma, idx, complex double );


    my_malloc_init( sigma_idx, cei.num_probe, int );
                                                                                  
    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        sigma_idx[iprobe - 1] = idx;
        idx_C = cei.probe_in_block[iprobe - 1];  /* block index */
        idx += pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C];
    }
                                                                                       
    my_malloc_init( sigma_all, idx, complex double );
                                                                                                

/*============== Allocate memory for tot, tott, g ====================*/

    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx_C = cei.probe_in_block[iprobe - 1];  /* block index */
        idx = max(idx, pmo.mxllda_cond[idx_C] * pmo.mxlocc_lead[iprobe-1]);
    }
                                                                                
    my_malloc_init( tot,  idx, complex double );
    my_malloc_init( tott, idx, complex double );
    my_malloc_init( g,    idx, complex double );
    my_malloc_init( ch0,  idx, complex double );
    my_malloc_init( ch01,  idx, complex double );
    my_malloc_init( ch10,  idx, complex double );
    my_malloc_init( H10,  idx,  double );
    my_malloc_init( S10,  idx,  double );


    my_malloc_init( green_C, pmo.ntot_low, complex double );
    /*st1 = (E_POINTS + NPES - 1) / NPES;*/
    st1 = (E_POINTS + pmo.npe_energy - 1) / pmo.npe_energy;
   
    my_malloc_init( Green_store, st1 * ntot, rmg_double_t );

/*===================================================================*/

    nx1 = cei.dos_window_start[0] * get_FG_RATIO();
    nx2 = cei.dos_window_end[0] * get_FG_RATIO();
    ny1 = cei.dos_window_start[1] * get_FG_RATIO();
    ny2 = cei.dos_window_end[1] * get_FG_RATIO();
    nz1 = cei.dos_window_start[2] * get_FG_RATIO();
    nz2 = cei.dos_window_end[2] * get_FG_RATIO();
                                                                                              
                                                                                              
                                                                                              
                                                                                              
    my_malloc_init( rho_energy, E_POINTS * get_FNX_GRID(), rmg_double_t );
    if (cei.num_probe > 2)
        my_malloc_init( rho_energy2, E_POINTS * get_FNY_GRID(), rmg_double_t );
                                                                                              
                                                                                              
                                                                                              
    xoff = get_FPX_OFFSET();
    yoff = get_FPY_OFFSET();
    zoff = get_FPZ_OFFSET();


/*===================================================================*/



    idx_e = 0;
    /*for (iene = pct.gridpe; iene < E_POINTS; iene += NPES)*/
    for (iene = pmo.myblacs; iene < E_POINTS; iene += pmo.npe_energy)
    {
        ene = emin + iene * de + I * E_imag;
        printf ("\n energy point %d %f +i %f\n", iene, creal(ene), cimag(ene));


        /* sigma is a complex matrix with dimension ct.num_states * ct.num_states 
         * it sums over all probes
         * tot, tott, green_tem is also a complex matrix, 
         * their memory should be the maximum of probe dimensions, lcr[1,...].num_states
         */
        for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
        {



            desca = &pmo.desc_lead[ (iprobe-1) * DLEN];

            numst = lcr[iprobe].num_states;

            PDTRAN(&numst, &numst, &one, lcr[iprobe].S01, &ione,
                    &ione, desca,
                    &zero, S10, &ione, &ione, desca);
            PDTRAN(&numst, &numst, &one, lcr[iprobe].H01, &ione,
                    &ione, desca,
                    &zero, H10, &ione, &ione, desca);

            idx = pmo.mxllda_lead[iprobe-1] * pmo.mxlocc_lead[iprobe-1];
            for (i = 0; i < idx; i++)
            {
                ch0[i] = ene * lcr[iprobe].S00[i] - Ha_eV * lcr[iprobe].H00[i];
                ch01[i] = ene * lcr[iprobe].S01[i] - Ha_eV * lcr[iprobe].H01[i];
                ch10[i] = ene * S10[i] - Ha_eV * H10[i];
            }


            Stransfer_p (tot, tott, ch0, ch01, ch10,iprobe);

            Sgreen_p (tot, tott, ch0, ch01, g, iprobe);


            idx_C = cei.probe_in_block[iprobe - 1];  /* block index
                                                      */
            idx = pmo.mxllda_cond[idx_C] * pmo.mxlocc_lead[iprobe-1];
            for (i = 0; i < idx; i++)
            {
                ch01[i] = ene * lcr[iprobe].SCL[i] - Ha_eV * lcr[iprobe].HCL[i];
            }

            desca = &pmo.desc_cond_lead[ (idx_C + (iprobe-1) * ct.num_blocks) * DLEN];
            descb = &pmo.desc_lead_cond[ (idx_C + (iprobe-1) * ct.num_blocks) * DLEN];
            numst = lcr[iprobe].num_states;
            numstC = ct.block_dim[idx_C];


            PDTRAN(&numst, &numstC, &one, lcr[iprobe].SCL, &ione, &ione, desca,
                    &zero, S10, &ione, &ione, descb);
            PDTRAN(&numst, &numstC, &one, lcr[iprobe].HCL, &ione, &ione, desca,
                    &zero, H10, &ione, &ione, descb);
            idx = pmo.mxllda_lead[iprobe -1] *
                pmo.mxlocc_cond[idx_C];
            for (i = 0; i < idx; i++)
            {
                ch10[i] = ene * S10[i] - Ha_eV * H10[i];
            }

            Sigma_p (sigma, ch0, ch01, ch10, g, iprobe);



            for (i = 0; i < pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C]; i++)
            {
                sigma_all[sigma_idx[iprobe - 1] + i] = sigma[i];
            }


        }                       /*  end for iprobe */




        /*Sgreen_c_wang (lcr[0].Htri, lcr[0].Stri, sigma_all, sigma_idx, 
          eneR, eneI, (complex double *) green_C, nC);*/
        Sgreen_c_p (lcr[0].Htri, lcr[0].Stri, sigma_all, sigma_idx, 
                ene, green_C); 


        for (st1 = 0; st1 < ntot; st1++)
        {
            Green_store[idx_e + st1] = -cimag(green_C[st1]);
            /*printf (" checkit  = %d %d %f \n", iene, idx + st1, Green_store[idx + st1]);*/
        }
        idx_e += ntot;

    }                           /*  end for iene */




    /*===================================================================*/


    /*for (iene = pmo.myblacs; iene < E_POINTS; iene += pmo.npe_energy)*/
    for (iene = 0; iene < E_POINTS; iene++)
    {
        printf ("hello .... %d\n", iene);

        root_pe = iene % pmo.npe_energy;
        idx = iene / pmo.npe_energy;


        for (st1 = 0; st1 < ntot; st1++)
        {
            lcr[0].density_matrix_tri[st1] = Green_store[idx * ntot + st1];
        }


        idx = ntot;
        MPI_Bcast (lcr[0].density_matrix_tri, idx, MPI_DOUBLE, root_pe,
                COMM_EN1);

        get_new_rho_local (states_distribute, rho);
        //get_new_rho_soft (states, rho);


        for (ix = 0; ix < get_FPX0_GRID(); ix++)
        {
            for (iy = 0; iy < get_FPY0_GRID(); iy++)
            {
                if((iy + yoff) >= ny1 && (iy + yoff) <= ny2)
                {
                    for (iz = 0; iz < get_FPZ0_GRID(); iz++)
                    {
                        if((iz + zoff) >= nz1 && (iz + zoff) <= nz2)
                        {
                            idx = iz + iy * get_FPZ0_GRID() + ix * FPYZ;
                            rho_energy[iene * get_FNX_GRID() + ix + xoff] += rho[idx];
                        }
                    }
                }
            }
        }


        if (cei.num_probe > 2)
        {
            for (iy = 0; iy < get_FPY0_GRID(); iy++)
            {
                for (ix = 0; ix < get_FPX0_GRID(); ix++)
                {
                    if((ix + xoff) >= nx1 && (ix + xoff) <= nx2)
                    {
                        for (iz = 0; iz < get_FPZ0_GRID(); iz++)
                        {
                            if((iz + zoff) >= nz1 && (iz + zoff) <= nz2)
                            {
                                idx = iz + iy * get_FPZ0_GRID() + ix * FPYZ;
                                rho_energy2[iene * get_FNY_GRID() + iy + yoff] += rho[idx];
                            }
                        }
                    }
                }
            }
        }

        my_barrier ();
    }



    iene = E_POINTS * get_FNX_GRID();
    global_sums (rho_energy, &iene, pct.grid_comm);
    if (pct.gridpe == 0)
    {
        double dx = get_celldm(0) / get_NX_GRID();
        double x0 = 0.5 * get_celldm(0);

        file = fopen ("dos.dat", "w");
        fprintf (file, "#     x[a0]      E[eV]          dos\n\n");
        for (iene = 0; iene < E_POINTS; iene++)
        {

            for (ix = 0; ix < get_NX_GRID(); ix++)
            {

                fprintf (file, " %10.6f %10.6f %12.6e\n",
                        ix * dx - x0, emin+iene*de, rho_energy[iene * get_FNX_GRID() + ix * get_FG_RATIO()]);
            }
            fprintf (file, "\n");
        }

        fclose (file);
    }

    if (cei.num_probe > 2)
    {

        iene = E_POINTS * get_FNY_GRID();
        global_sums (rho_energy2, &iene, pct.grid_comm);
        if (pct.gridpe == 0)
        {
            double y = get_celldm(1) * get_celldm(0);
            double dy = y / get_NY_GRID();
            double y0 = 0.5 * y;

            file = fopen ("dos2.dat", "w");
            fprintf (file, "#     y[b0]      E[eV]          dos\n\n");
            for (iene = 0; iene < E_POINTS; iene++)
            {

                for (iy = 0; iy < get_NY_GRID(); iy++)
                {

                    fprintf (file, " %10.6f %10.6f %12.6e\n",
                            iy * dy - y0, emin+iene*de, rho_energy2[iene * get_FNY_GRID() + iy * get_FG_RATIO()]);
                }
                fprintf (file, "\n");
            }

            fclose (file);
        }
        my_free(rho_energy2);
    }


    my_barrier ();
    fflush (NULL);

    /*===============================*/
    my_free(tot);
    my_free(tott);
    my_free(g);
    my_free(sigma_all);
    my_free(sigma_idx);
    my_free(green_C);

    my_free(lcr[0].Htri);
    my_free(lcr[0].Stri);
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        my_free(lcr[iprobe].H00);
        my_free(lcr[iprobe].S00);
        my_free(lcr[iprobe].H01);
        my_free(lcr[iprobe].S01);
    }

    /*===============================*/

    my_free(rho_energy); 
    my_free(Green_store);
    my_free(ch0);
    my_free(ch01);
    my_free(ch10);
    my_free(H10);
    my_free(S10);



}
