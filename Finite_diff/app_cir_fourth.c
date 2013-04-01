/************************** SVN Revision Information **************************
 **    $Id: app_cir_sixth.c 1722 2012-05-07 19:38:37Z ebriggs $    **
******************************************************************************/

#include "main.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#if HYBRID_MODEL
#include "hybrid.h"
#endif

static void app_cir_fourth_global (REAL * a, REAL * b);

void app_cir_fourth (REAL * a, REAL * b, int dimx, int dimy, int dimz)
{

    int ix, iy, iz, numgrid, tid;
    int ixs, iys, ixms, ixps, iyms, iyps;
    int incy, incx;
    int incyr, incxr;
    REAL *rptr;
    REAL c000, c100;

    if((ct.ibrav != CUBIC_PRIMITIVE) && (ct.ibrav != ORTHORHOMBIC_PRIMITIVE)) {
        error_handler("Grid symmetry not programmed yet in app_cir_fourth.\n");
    }

#if HYBRID_MODEL
    tid = get_thread_tid();
    if(tid < 0) tid = 0;  // OK in this case
#else
    tid = 0;
#endif

#if 0
#if GPU_FD_ENABLED
    rmg_double_t *gpu_a, *gpu_b;
    cudaStream_t *cstream;
    int pbasis = dimx * dimy * dimz;
    int sbasis = (dimx + 2) * (dimy + 2) * (dimz + 2);

    // cudaMallocHost is painfully slow so we use a pointers into regions that were previously allocated.
    rptr = &ct.gpu_host_fdbuf2[tid * sbasis];
    gpu_a = &ct.gpu_work3[tid * sbasis];
    gpu_b = &ct.gpu_work4[tid * sbasis];

    cstream = get_thread_cstream();
    trade_imagesx (a, rptr, dimx, dimy, dimz, 1, CENTRAL_FD);

    cudaMemcpyAsync( gpu_a, rptr, sbasis * sizeof(rmg_double_t), cudaMemcpyHostToDevice, *cstream);

    app_cir_fourth_gpu (gpu_a, gpu_b, dimx, dimy, dimz, *cstream);
    cudaMemcpyAsync(b, gpu_b, pbasis * sizeof(rmg_double_t), cudaMemcpyDeviceToHost, *cstream);

    return;
#endif
#endif

    numgrid = dimx * dimy * dimz;
    if(numgrid == pct.P0_BASIS) {
        app_cir_fourth_global (a, b);
        return;
    }

    incx = (dimz + 2) * (dimy + 2);
    incy = dimz + 2;
    incxr = dimz * dimy;
    incyr = dimz;

    my_malloc (rptr, (dimx + 2) * (dimy + 2) * (dimz + 2), REAL);

    trade_imagesx (a, rptr, dimx, dimy, dimz, 1, CENTRAL_FD);

    c000 = 0.5;
    c100 = 1.0 / 12.0;
    for (ix = 1; ix < dimx + 1; ix++)
    {
        ixs = ix * incx;
        ixms = (ix - 1) * incx;
        ixps = (ix + 1) * incx;

        for (iy = 1; iy < dimy + 1; iy++)
        {
            iys = iy * incy;
            iyms = (iy - 1) * incy;
            iyps = (iy + 1) * incy;

            for (iz = 1; iz < dimz + 1; iz++)
            {

                b[(ix - 1) * incxr + (iy - 1) * incyr + (iz - 1)] =
                    c100 * (rptr[ixs + iys + (iz - 1)] +
                            rptr[ixs + iys + (iz + 1)] +
                            rptr[ixms + iys + iz] +
                            rptr[ixps + iys + iz] +
                            rptr[ixs + iyms + iz] +
                            rptr[ixs + iyps + iz]) + 
                    c000 *  rptr[ixs + iys + iz];

            }                   /* end for */

        }                       /* end for */

    }                           /* end for */

    my_free (rptr);
}

void app_cir_fourth_global (REAL * a, REAL * b)
{

    int ix, iy, iz;
    int ixs, iys, ixms, ixps, iyms, iyps;
    int incy, incx;
    int incyr, incxr;
    REAL *rptr, rz, rzps, rzms, rzpps;
    REAL c000, c100;

    incx = (FIXED_ZDIM + 2) * (FIXED_YDIM + 2);
    incy = FIXED_ZDIM + 2;
    incxr = FIXED_ZDIM * FIXED_YDIM;
    incyr = FIXED_ZDIM;

    my_malloc (rptr, (FIXED_XDIM + 2) * (FIXED_YDIM + 2) * (FIXED_ZDIM + 2), REAL);

    trade_imagesx (a, rptr, FIXED_XDIM, FIXED_YDIM, FIXED_ZDIM, 1, CENTRAL_FD);

    c000 = 0.5;
    c100 = 1.0 / 12.0;


    for (ix = 1; ix < FIXED_XDIM + 1; ix++)
    {
        ixs = ix * incx;
        ixms = (ix - 1) * incx;
        ixps = (ix + 1) * incx;

        for (iy = 1; iy < FIXED_YDIM + 1; iy++)
        {
            iys = iy * incy;
            iyms = (iy - 1) * incy;
            iyps = (iy + 1) * incy;

            for (iz = 1; iz < FIXED_ZDIM + 1; iz++)
            {

                b[(ix - 1) * incxr + (iy - 1) * incyr + (iz - 1)] =
                    c100 * (rptr[ixs + iys + (iz - 1)] +
                            rptr[ixs + iys + (iz + 1)] +
                            rptr[ixms + iys + iz] +
                            rptr[ixps + iys + iz] +
                            rptr[ixs + iyms + iz] +
                            rptr[ixs + iyps + iz]) + 
                    c000 * rptr[ixs + iys + iz];

            }                   /* end for */

        }                       /* end for */

    }                           /* end for */


    my_free (rptr);
    return;

}
