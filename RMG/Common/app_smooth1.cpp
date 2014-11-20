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


#include "rmgtypes.h"
#include <complex>


/**

 Smooths f and returns result in work
*/
template <typename RmgType>
void CPP_app_smooth1 (RmgType * f, RmgType * work, int dimx, int dimy, int dimz);

template void CPP_app_smooth1<std::complex<float> > (std::complex<float> *, std::complex<float> *, int , int , int );

template <typename RmgType>
void CPP_app_smooth1 (RmgType * f, RmgType * work, int dimx, int dimy, int dimz)
{

    int iz, ix, iy, incx, incy;
    int ixs, iys, ixms, ixps, iyms, iyps;

    RmgType scale, fc, cc, temp;

    incy = dimz + 2;
    incx = (dimz + 2) * (dimy + 2);

    cc = 6.0;
    fc = 1.0;
    scale = 1.0 / 12.0;


    for (ix = 1; ix <= dimx; ix++)
    {

        ixs = ix * incx;
        ixms = (ix - 1) * incx;
        ixps = (ix + 1) * incx;

        for (iy = 1; iy <= dimy; iy++)
        {

            iys = iy * incy;
            iyms = (iy - 1) * incy;
            iyps = (iy + 1) * incy;

            for (iz = 1; iz <= dimz; iz++)
            {

                temp = cc * f[ixs + iys + iz] +
                    fc * (f[ixms + iys + iz] +
                          f[ixps + iys + iz] +
                          f[ixs + iyms + iz] +
                          f[ixs + iyps + iz] +
                          f[ixs + iys + (iz - 1)] +
                          f[ixs + iys + (iz + 1)]);

                work[ixs + iys + iz] = scale * temp;

            }                   /* end for */

        }                       /* end for */

    }                           /* end for */

}                               /* end app_smooth1 */



extern "C" void app_smooth1 (rmg_double_t * f, rmg_double_t * work, int dimx, int dimy, int dimz)
{
    CPP_app_smooth1<rmg_double_t> (f, work, dimx, dimy, dimz);
}

extern "C" void app_smooth1_f (rmg_float_t * f, rmg_float_t * work, int dimx, int dimy, int dimz)
{
    CPP_app_smooth1<rmg_float_t> (f, work, dimx, dimy, dimz);
}
