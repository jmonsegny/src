/* Bandpass filtering.
*/
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

#include "butterworth.h"

static void reverse (int n1, float* trace);

int main (int argc, char* argv[]) 
{
    bool phase, verb;
    int i2, n1, n2, nplo, nphi;
    float d1, flo, fhi, *trace;
    const float eps=0.0001;
    sf_file in, out;

    sf_init (argc, argv); 
    in = sf_input("in");
    out = sf_output("out");
    
    if (!sf_histint(in,"n1",&n1)) sf_error("No n1= in input");
    n2 = sf_leftsize(in,1);

    if (!sf_histfloat(in,"d1",&d1)) sf_error("No d1= in input");

    if (SF_FLOAT != sf_gettype(in)) sf_error("Need float input");

    if (!sf_getfloat("flo",&flo)) {
	/* Low frequency in band, default is 0 */
	flo=0.;
    } else if (0. > flo) {
	sf_error("Negative flo=%g",flo);
    } else {
	flo *= d1;
    }

    if (!sf_getfloat("fhi",&fhi)) {
	/* High frequency in band, default is Nyquist */	
	fhi=0.5;
    } else {
	fhi *= d1;	
	if (flo > fhi) 
	    sf_error("Need flo < fhi, "
		     "got flo=%g, fhi=%g",flo/d1,fhi/d1);
	if (0.5 < fhi)
	    sf_error("Need fhi < Nyquist, "
		     "got fhi=%g, Niquist=%g",fhi/d1,0.5/d1);
    }
	   
    if (!sf_getint("nplo",&nplo)) nplo = 3;
    /* number of poles for low cutoff */
    else if (nplo < 1)            nplo = 1;
    else if (nplo > 1)            nplo /= 2; 

    if (!sf_getint("nphi",&nphi)) nphi = 3;
    /* number of poles for high cutoff */
    else if (nphi < 1)            nphi = 1;
    else if (nphi > 1)            nphi /= 2; 

    if (!sf_getbool("phase",&phase)) phase=false;    
    /* y: minimum phase, n: zero phase */
    if (!sf_getbool("verb",&verb)) verb=false;
    /* verbosity flag */

    if (verb) sf_warning("flo=%g fhi=%g nplo=%d nphi=%d",
			 flo,fhi,nplo,nphi);
    
    trace = sf_floatalloc(n1);

    for (i2=0; i2 < n2; i2++) {
	sf_floatread(trace,n1,in);

	if (flo > eps) {
	    bfhighpass (nplo, flo, n1, trace, trace);

	    if (!phase) {
		reverse (n1, trace);
		bfhighpass (nplo, flo, n1, trace, trace);
		reverse (n1, trace);
	    }
	}

	if (fhi < 0.5-eps) {    
	    bflowpass (nphi, fhi, n1, trace, trace);

	    if (!phase) {
		reverse (n1, trace);
		bflowpass (nphi, fhi, n1, trace, trace);
		reverse (n1, trace);
	    }
	}
 
	sf_floatwrite(trace,n1,out);
    }

    exit (0);
}


static void reverse (int n1, float* trace) {
    int i1;
    float t;

    for (i1=0; i1 < n1/2; i1++) { 
	t=trace[i1];
	trace[i1]=trace[n1-1-i1];
	trace[n1-1-i1]=t;
    }
}

/* 	$Id$	 */
