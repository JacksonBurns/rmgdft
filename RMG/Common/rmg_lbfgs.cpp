/*
 *
 * Copyright 2014 The RMG Project Developers. See the COPYRIGHT file 
 * at the top-level directory of this distribution or in the current
 * directory.
 * 
 * This file is part of RMG. 
 * RMG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RMG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/



#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "transition.h"
#include "bfgs.h"

void rmg_lbfgs (void)
{
    int ion, fpt;
    ION *iptr;
    double *position, *force;

    //my_malloc(position, 3*ct.num_ions, double);
    //my_malloc(force, 3*ct.num_ions, double);
    position = new double[3*ct.num_ions];
    force = new double[3*ct.num_ions];
    fpt = ct.fpt[0];

    /* Loop over ions */
    for (ion = 0; ion < ct.num_ions; ion++)
    {

        /* Get ion pointer */
        iptr = &Atoms[ion];
        force[ion * 3 + 0] = iptr->force[fpt][0];
        force[ion * 3 + 1] = iptr->force[fpt][1];
        force[ion * 3 + 2] = iptr->force[fpt][2];

        position[ion * 3 + 0] = iptr->crds[0];
        position[ion * 3 + 1] = iptr->crds[1];
        position[ion * 3 + 2] = iptr->crds[2];
    }

    int num_images = pct.images;
    num_images = 1;
    lbfgs (position, force, ct.num_ions, num_images);

    for (ion = 0; ion < ct.num_ions; ion++)
    {

        iptr = &Atoms[ion];
        if(iptr->movable[0] )
            iptr->crds[0] = position[ion * 3 + 0] ;
        if(iptr->movable[1] )
            iptr->crds[1] = position[ion * 3 + 1] ;
        if(iptr->movable[2] )
            iptr->crds[2] = position[ion * 3 + 2] ;

        to_crystal (iptr->xtal, iptr->crds);
        if (iptr->xtal[0] > ONE)
            iptr->xtal[0] -= ONE;
        if (iptr->xtal[0] < ZERO)
            iptr->xtal[0] += ONE;

        if (iptr->xtal[1] > ONE)
            iptr->xtal[1] -= ONE;
        if (iptr->xtal[1] < ZERO)
            iptr->xtal[1] += ONE;

        if (iptr->xtal[2] > ONE)
            iptr->xtal[2] -= ONE;
        if (iptr->xtal[2] < ZERO)
            iptr->xtal[2] += ONE;

        to_cartesian (iptr->xtal, iptr->crds);

    }

    delete [] force;
    delete [] position;

}                               /* end rmg_lbfgs */


// Wrapper for simple case. Ionic relaxation only.
void simple_lbfgs (void)
{
    double nelec, felec, fcp_thr, fcp_hess, fcp_error, fcp_cap;
    double energy_error, grad_error, cell_error;
    double *fcell, *iforceh;
    int lfcp = false, lmovecell = false;
    int step_accepted, stop_bfgs, failed;
    double energy = ct.TOTAL;
    double energy_thr = 1.0e-4;
    double grad_thr = 1.0e-4;
    double cell_thr = 0.5 / RY_KBAR;
    double *h = new double[9]();
    int istep;

    double *position = new double[3*ct.num_ions]();
    double *force = new double[3*ct.num_ions]();

    // Need to check for other symmetries
    for(int i=0;i < 3;i++)
    {
        h[i] = Rmg_L.a0[i];
        h[i+3] = Rmg_L.a1[i];
        h[i+6] = Rmg_L.a2[i];
    }

    int fpt = ct.fpt[0];
    /* Loop over ions */
    double alat = Rmg_L.a0[0];
    for (int ion = 0; ion < ct.num_ions; ion++)
    {

        /* Get ion pointer */
        ION *iptr = &Atoms[ion];
        force[ion*3+0] = -2.0*alat*iptr->force[fpt][0];
        force[ion*3+1] = -2.0*alat*iptr->force[fpt][1];
        force[ion*3+2] = -2.0*alat*iptr->force[fpt][2];

        position[ion * 3 + 0] = iptr->xtal[0];
        position[ion * 3 + 1] = iptr->xtal[1];
        position[ion * 3 + 2] = iptr->xtal[2];
    }

    if(pct.gridpe==0)
    {
        bfgs( &ct.num_ions, position, h, &nelec, &energy,
               force, fcell, iforceh, &felec,
               &energy_thr, &grad_thr, &cell_thr, &fcp_thr,
               &energy_error, &grad_error, &cell_error, &fcp_error,
               &lmovecell, &lfcp, &fcp_cap, &fcp_hess, &step_accepted,
               &stop_bfgs, &failed, &istep );
    }
    // Send updated positions to other nodes. May need to update for images
    // and symmetry considerations for non-gamma.
    MPI_Bcast(position, 3*ct.num_ions, MPI_DOUBLE, 0, pct.grid_comm);

    //printf("BBBB  %d  %d  %d  %12.6f\n",step_accepted, stop_bfgs, failed, energy_error);

    // Copy back
    for (int ion = 0; ion < ct.num_ions; ion++)
    {

        ION *iptr = &Atoms[ion];
        if(iptr->movable[0] )
            iptr->xtal[0] = position[ion * 3 + 0] ;
        if(iptr->movable[1] )
            iptr->xtal[1] = position[ion * 3 + 1] ;
        if(iptr->movable[2] )
            iptr->xtal[2] = position[ion * 3 + 2] ;

        if (iptr->xtal[0] > ONE)
            iptr->xtal[0] -= ONE;
        if (iptr->xtal[0] < ZERO)
            iptr->xtal[0] += ONE;

        if (iptr->xtal[1] > ONE)
            iptr->xtal[1] -= ONE;
        if (iptr->xtal[1] < ZERO)
            iptr->xtal[1] += ONE;

        if (iptr->xtal[2] > ONE)
            iptr->xtal[2] -= ONE;
        if (iptr->xtal[2] < ZERO)
            iptr->xtal[2] += ONE;

        to_cartesian (iptr->xtal, iptr->crds);

    }
    delete [] force;
    delete [] position;

}
