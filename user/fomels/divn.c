/* N-dimensional smooth division */
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>

static int niter, n;
static float *p, eps;

void divn_init(int ndim   /* number of dimensions */, 
	       int nd     /* data size */, 
	       int *ndat  /* data dimensions [ndim] */, 
	       int *nbox  /* smoothing radius [ndim] */, 
	       int niter1 /* number of iterations */,
	       float eps1 /* regularization */,
	       bool verb  /* verbosity */) 
/*< initialize >*/
{
    niter = niter1;
    n = nd;
    eps = eps1;

    sf_trianglen_init(ndim, nbox, ndat);
    sf_conjgrad_init(nd, nd, nd, nd, 1., 1.e-6, verb, false);
    p = sf_floatalloc (nd);
}

void divn_close (void)
/*< free allocated storage >*/
{
    sf_trianglen_close();
    sf_conjgrad_close();
    free (p);
}

void divn (float* num, float* den,  float* rat)
/*< smoothly divide rat=num/den >*/
{
    int i;
    double norm;

    if (eps > 0.0f) {
	for (i=0; i < n; i++) {
	    norm = 1.0/hypot(den[i],eps);

	    num[i] *= norm;
	    den[i] *= norm;
	}
    } 

    norm = sqrt(n/cblas_dsdot(n,den,1,den,1));

    for (i=0; i < n; i++) {
	num[i] *= norm;
	den[i] *= norm;
    }

    sf_weight_init(den);
    sf_conjgrad(NULL, sf_weight_lop,sf_trianglen_lop,p,rat,num,niter); 
}

void divn_combine (const float* one, const float* two, float *prod)
/*< compute product of two divisions >*/
{
    int i;
    float p;

    for (i=0; i < n; i++) {
	p = sqrtf(fabsf(one[i]*two[i]));
	if ((one[i] > 0. && two[i] < 0. && -two[i] >= one[i]) ||
	    (one[i] < 0. && two[i] > 0. && two[i] >= -one[i])) 
	    p = -p;
	p += 1.;
	p *= p;
	p *= p/16.;
	prod[i] = p;	
    }
}
    

/* 	$Id: divn.c 7873 2011-11-13 14:44:04Z sfomel $	 */
