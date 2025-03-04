/****f* QMD-MG->FT/pack_vhstod.c *****
 * NAME
 *   Ab initio real space code with multigrid acceleration
 *   Quantum molecular dynamics package.
 *   Version: 2.1.5
 * COPYRIG->T
 *   Copyright (C) 1995  Emil Briggs
 *   Copyright (C) 1998  Emil Briggs, Charles Brabec, Mark Wensell, 
 *                       Dan Sullivan, Chris Rapcewicz, Jerzy Bernholc
 *   Copyright (C) 2001  Emil Briggs, Wenchang Lu,
 *                       Marco Buongiorno Nardelli,Charles Brabec, 
 *                       Mark Wensell,Dan Sullivan, Chris Rapcewicz,
 *                       Jerzy Bernholc
 * FUNCTION
 *   void pack_vhstod(double *s, double *d, int dimx, int dimy, int dimz)
 *   Used to pack grids when computing the hartree potential 
 *   For periodic system, d = s
 *   For surface system,  d[x,y,Nz/2+1:Nz/2+Nz] = s[x,y,1:Nz]
 *   FOr cluster system:  d[Nx/2+1:Nx/2+Nx, ...] = s[1:Nx, 1:Ny, 1:Nz] 
 * INPUTS
 *   s[dmix*dimy*dimz]
 *   dimx, dimy, dimz: dimension of the wave function grid
 * OUTPUT
 *   d[ct.vh_pxgrid * ct.vh_pygrid * ct.vh_pzgrid]
 *       see init.c for ct.vh_pxgrid, ...
 *       array in the hatree potential grid
 * PARENTS
 *   get_vh.c
 * CHILDREN
 *   nothing
 * SOURCE
 */

#include "BaseGrid.h"
#include "boundary_conditions.h"
#include "packfuncs.h"


/* This function is used to pack grids when computing the hartree potential */
void CPP_pack_stod (BaseGrid *G, double * s, double * d, int dimx, int dimy, int dimz, int boundaryflag)
{
    int ix, iy, iz;
    int pex, pey, pez;
    int sxlo, sylo, szlo;
    int sxhi, syhi, szhi;
    int dxlo, dylo, dzlo;
    int dxhi, dyhi, dzhi;
    int loidx, hiidx;
    int hix, hiy, hiz;
    int hincx, hincy;

    if (boundaryflag == PERIODIC)
    {

        for (ix = 0; ix < (dimx * dimy * dimz); ix++)
            d[ix] = s[ix];
        return;

    }                           /* end if */

    G->pe2xyz (G->get_rank(), &pex, &pey, &pez);
    sxlo = pex * dimx;
    sylo = pey * dimy;
    szlo = pez * dimz;
    sxhi = sxlo + dimx - 1;
    syhi = sylo + dimy - 1;
    szhi = szlo + dimz - 1;

    dxlo = pex * 2 * dimx - G->get_NX_GRID(1) / 2;
    dylo = pey * 2 * dimy - G->get_NY_GRID(1) / 2;
    dzlo = pez * 2 * dimz - G->get_NZ_GRID(1) / 2;
    dxhi = dxlo + 2 * dimx - 1;
    dyhi = dylo + 2 * dimy - 1;
    dzhi = dzlo + 2 * dimz - 1;
    hincy = 2 * dimz;
    hincx = 4 * dimy * dimz;

    if (boundaryflag == SURFACE)
    {

        dxlo = sxlo;
        dylo = sylo;
        dxhi = sxhi;
        dyhi = syhi;
        hincx = 2 * dimy * dimz;

    }                           /* end if */


    loidx = 0;
    for (ix = sxlo; ix <= sxhi; ix++)
    {

        for (iy = sylo; iy <= syhi; iy++)
        {

            for (iz = szlo; iz <= szhi; iz++)
            {


                /* Check if we map into the double grid */
                if ((ix >= dxlo) && (ix <= dxhi) &&
                    (iy >= dylo) && (iy <= dyhi) && (iz >= dzlo) && (iz <= dzhi))
                {

                    hix = ix - dxlo;
                    hiy = iy - dylo;
                    hiz = iz - dzlo;
                    hiidx = hix * hincx + hiy * hincy + hiz;

                    /* Yes there is a mapping so transfer the data */
                    d[hiidx] = s[loidx];

                }               /* end if */

                loidx++;

            }                   /* end for */

        }                       /* end for */

    }                           /* end for */

}                               /* end pack_stod */


