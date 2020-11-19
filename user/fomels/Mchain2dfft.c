/* Find a symmetric chain of 2D-Fourier weighting and scaling with movies*/
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
#include "chain2dfft.h"
#include "twosmooth2.h"
#include "fft2.h"



int main(int argc, char* argv[])
{
    int i, n, nk, n2, iter, niter, liter, snap, nt, nx, nt1, nt2, nx2;
    int rect1, rect2, frect1, frect2; 
    float dt,dx,x0;
    float l2_r,l2_r_new,alpha;
    float *w, *dw, *x, *y, *r, *p, *lsmig,*r_new, *w_prev;
    sf_file wht, fwht, src, tgt, mch;
    sf_file w0, wf0;
    /*sf_file for w, wf, lsmig snapshot*/
    sf_file snap_w = NULL, snap_wf = NULL, snap_lsmig = NULL;
    /* For fft2 */
    bool isCmplx = false;
    int pad = 1;

    sf_init(argc,argv);
    src = sf_input("in");
    wht = sf_output("out"); 
    /* space weight */

    tgt = sf_input("target");
    /* target */

    w0 = sf_input("init_w");
    wf0 = sf_input("init_wf");

    fwht = sf_output("fweight"); 
    /* frequency weight */
    mch = sf_output("match"); 
    /* matched */


    if (!sf_histint(src,"n1",&nt)) sf_error("No n1= in input");
    if (!sf_histint(src,"n2",&nx)) sf_error("No n2= in input");
    if (!sf_histfloat(src,"d1",&dt)) sf_error("No d1= in input");
    if (!sf_histfloat(src,"d2",&dx)) sf_error("No d2= in input");
    if (!sf_histfloat(src,"o2",&x0)) x0=0.; 

    if (!sf_getint("niter",&niter)) niter=0;
    /* number of iterations */
    if (!sf_getint("liter",&liter)) liter=50;
    /* number of linear iterations */


    n = nt*nx; 

    nk = fft2_init(isCmplx, pad, nt, nx, &nt1, &nx2);
    nt2 = nk/nx2;
 
    n2 = 3*n+nk; 
    sf_putint(fwht,"n1",nt2);
    sf_putint(fwht,"n2",nx2);

    w = sf_floatalloc(n2); 
    dw = sf_floatalloc(n2);
    w_prev = sf_floatalloc(n2);

    x = sf_floatalloc(n); 
    y = sf_floatalloc(n);
    lsmig = sf_floatalloc(n); /* decon image */
    r = sf_floatalloc(3*n);
    r_new = sf_floatalloc(3*n);


    /* I/O Setup for snapshot */

    if (!sf_getint("snap",&snap)) snap=0;
    /* interval for snapshots */

    if (snap > 0) {
    snap_w = sf_output("wsnap");
    /* time weight movie */
    snap_wf = sf_output("wfsnap");
    /* frequency weight movie */

    snap_lsmig = sf_output("lsmigsnap");
    /* Deconvolved image movie */

    
    /* time/space weight */
    sf_putint(snap_w,"n1",nt);
    sf_putfloat(snap_w,"d1",dt);
    sf_putfloat(snap_w,"o1",0.);
    sf_putstring(snap_w,"label1","Depth");

    sf_putint(snap_w,"n2",nx);
    sf_putfloat(snap_w,"d2",dx);
    sf_putfloat(snap_w,"o2",x0);
    sf_putstring(snap_w,"label2","Distance");
    
    sf_putint(snap_w,"n3",niter);
    sf_putfloat(snap_w,"d3",1.0);
    sf_putfloat(snap_w,"o3",0.0);

    /* frequency weight */
    sf_putint(snap_wf,"n1",nt2);
    sf_putfloat(snap_wf,"d1",dt);
    sf_putfloat(snap_wf,"o1",0.);
    sf_putstring(snap_wf,"label1","Vertical wavenumber");

    sf_putint(snap_wf,"n2",nx2);
    sf_putfloat(snap_wf,"d2",dx);
    sf_putfloat(snap_wf,"o2",x0);
    sf_putstring(snap_wf,"label2","Horizontal wavenumber");

    sf_putint(snap_wf,"n3",niter);
    sf_putfloat(snap_wf,"d3",1.0);
    sf_putfloat(snap_wf,"o3",0.0);

    /* decon image */

    sf_putint(snap_lsmig,"n1",nt);
    sf_putfloat(snap_lsmig,"d1",dt);
    sf_putfloat(snap_lsmig,"o1",0.);
    sf_putstring(snap_lsmig,"label1","Depth");

    sf_putint(snap_lsmig,"n2",nx);
    sf_putfloat(snap_lsmig,"d2",dx);
    sf_putfloat(snap_lsmig,"o2",x0);
    sf_putstring(snap_lsmig,"label2","Distance");


    sf_putint(snap_lsmig,"n3",niter);
    sf_putfloat(snap_lsmig,"d3",1.0);
    sf_putfloat(snap_lsmig,"o3",0.0);
    }



    if (!sf_getint("rect1",&rect1)) rect1=1;
    /* smoothing in time dim1*/
    if (!sf_getint("rect2",&rect2)) rect2=1;
    /* smoothing in time dim2*/
    if (!sf_getint("frect1",&frect1)) frect1=1;
    /* smoothing in frequency dim1 */
    if (!sf_getint("frect2",&frect2)) frect2=1;
    /* smoothing in frequency dim2 */

    twosmooth2_init(n,nk,nt,nt2,
            rect1,rect2,
            frect1,frect2,
            2*n);

    sf_floatread(x,n,src); /* source */
    sf_floatread(y,n,tgt); /* target */

    sfchain2d_init(nt,nx,nt1,nx2,nk,
		   w+2*n,w+3*n,w,w+n,x);

    sf_conjgrad_init(n2, n2, 3*n, 3*n, 1., 1.e-6, true, false);
 

    p = sf_floatalloc(n2); 

    /* initialize w [time w and freqz w] */
    for (i=0; i < 2*n; i++) {
    w[i] = 0.0f; 
    }

    sf_floatread(w+2*n, n, w0);
    sf_floatread(w+3*n, nk, wf0);
   

    for (iter=0; iter < niter; iter++) {
    sf_warning("Start %d",iter);
    alpha=1.0;

    sfchain2d_res(y,r);
    l2_r = cblas_snrm2(3*n,r,1);
    sf_warning("(Before update) L2 norm of res: %g", l2_r);

    sf_warning("Residual %d",iter);
    
    sf_conjgrad(NULL, sfchain2d_lop,twosmooth2_lop,p,dw,r,liter);


    for (i=0; i < n2; i++) {
        w[i] += alpha*dw[i];
    }   

    sfchain2d_res(y,r_new);

    l2_r_new = cblas_snrm2(3*n,r_new,1);

    sf_warning("(After update) L2 norm of res: %g, alpha = %g", l2_r_new,alpha);


    while(l2_r_new>l2_r && alpha > 0.00001){ 
        sf_warning("Too big step !");    

        for (i=0; i < n2; i++) {
            w[i] = w_prev[i];
        }

        alpha *=0.5;

        for (i=0; i < n2; i++) {
            w[i] += alpha*dw[i];
        }

        sfchain2d_res(y,r_new);

        l2_r_new = cblas_snrm2(3*n,r_new,1);
        sf_warning("(After update) L2 norm of res: %g, alpha = %g", l2_r_new,alpha);


    }
    sf_warning("Pass now !");
    




    /* Snapshot of w, wf, deconvolved inmage */

    if(NULL != snap_w){
        sf_floatwrite(w+2*n, n, snap_w);
    }
    if(NULL != snap_wf){
        sf_floatwrite(w+3*n, nk, snap_wf);        
    }

    if(NULL != snap_lsmig){
    /* y is first migration (target) matched by m2 (src) */

        sfchain2d_deconimg(y , lsmig, w+2*n, w+3*n);
        sf_floatwrite(lsmig, n, snap_lsmig);        
    }



    }/* End of iteration */

    sf_floatwrite(w+2*n,n,wht); 
    sf_floatwrite(w+3*n,nk,fwht);
    sfchain2d_apply(y);
    sf_floatwrite(y,n,mch);

    exit(0);
}
