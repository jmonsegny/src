/*#include <math.h>*/
/*^*/
#include <rsf.h>
/*^*/

#include "hwt2d.h"

static axa az,ax,at,ag;
static float **vv;
static float  *ww;
/*------------------------------------------------------------*/

void hwt2d_init(float** vv_in    /* velocity */,
		axa     az_in    /* z axis   */,
		axa     ax_in    /* x axis   */,
		axa     at_in    /* t axis   */,
		axa     ag_in    /* g axis   */)
/*< initialize hwt2d >*/
{
    az = az_in;
    ax = ax_in;
    at = at_in;
    ag = ag_in;

    vv = vv_in;
    ww=sf_floatalloc(2*ag.n);
}

/*------------------------------------------------------------*/
double hwtgetv(pt2d P) 
/*< get velocity from map >*/
{
    float  z, x;
    float rz,rx;
    int   jz,jx;
    float fz,fx;
    float v;

    z = SF_MIN(SF_MAX(az.o,P.z),az.o+(az.n-2)*az.d);
    x = SF_MIN(SF_MAX(ax.o,P.x),ax.o+(ax.n-2)*ax.d);

    rz = (z-az.o)/az.d;
    jz = (int)(rz);
    fz =       rz-jz;

    rx = (x-ax.o)/ax.d;
    jx = (int)(rx);
    fx =       rx-jx;

    v = vv[ jx  ][ jz  ] * (1-fz)*(1-fx) +
	vv[ jx  ][ jz+1] * (  fz)*(1-fx) +
	vv[ jx+1][ jz  ] * (1-fz)*(  fx) +
	vv[ jx+1][ jz+1] * (  fz)*(  fx);

    return(v);
}

/*------------------------------------------------------------*/

bool hwtcusp(pt2d Qo, pt2d Pm, pt2d Po, pt2d Pp)
/*< find cusp points >*/
{
  int  sjm,sjp;

  sjm = SF_SIG( JAC2d(Po,Qo,Pm) );
  sjp = SF_SIG( JAC2d(Po,Qo,Pp) );

  if (sjm*sjp==1)
    return true;
  else
    return false;
}

/*------------------------------------------------------------*/

pt2d raytr(pt2d Qo, pt2d Po)
/*< ray tracing >*/
{
    pt2d Pm,Pp, Ro;
    float lo;
    double sina,cosa,tana;
    int    ss;

    lo = Po.v * at.d;

    if(Po.z!=Qo.z) {
	tana = (Po.x-Qo.x) / (Po.z-Qo.z);	
	ss   = SF_SIG(tana);

	tana = SF_ABS(tana);	
	sina = tana/sqrt(1+tana*tana);
	cosa =    1/sqrt(1+tana*tana);
    } else {
	ss = 1;

	cosa = 0;
	sina = 1;
    }

    Pm.x=Po.x-lo*cosa; Pm.z=Po.z+ss*lo*sina; Pm.v=hwtgetv(Pm);
    Pp.x=Po.x+lo*cosa; Pp.z=Po.z-ss*lo*sina; Pp.v=hwtgetv(Pp);

    Ro=hwtstep(Qo,Pm,Po,Pp);
    return(Ro);
}

/*------------------------------------------------------------*/

pt2d wfttr(pt2d Qo, pt2d Pm, pt2d Po, pt2d Pp)
/*< wavefront tracing >*/
{
    pt2d Ro;
    Ro=hwtstep(Qo,Pm,Po,Pp);
    return(Ro);
}

/*------------------------------------------------------------*/

pt2d hwtstep(pt2d Qo, pt2d Pm, pt2d Po, pt2d Pp)
/*< one HWT time step >*/
{
    pt2d   Ro,Sm,Sp;
    double a,b,c;
    float  lo;
    double ka, kb;

    a= Pm.x-Pp.x;
    b= Pm.z-Pp.z;
    c=(Pm.v-Pp.v)*at.d;

    lo = Po.v * at.d; /* circle radius at Po */

    ka =                            c /(a*a+b*b);
    kb = SF_SIG(a) * sqrt(a*a+b*b-c*c)/(a*a+b*b); 

    Sm.x = Po.x - lo*( ka*a - kb*b );
    Sm.z = Po.z - lo*( ka*b + kb*a );
    
    Sp.x = Po.x - lo*( ka*a + kb*b );
    Sp.z = Po.z - lo*( ka*b - kb*a );

    Ro = DST2d(Sm,Qo) > DST2d(Sp,Qo) ? Sm : Sp;
    Ro.v=hwtgetv(Ro);
    return(Ro);
}

/*------------------------------------------------------------*/

pt2d hwtorth( pt2d Pm, pt2d Po, pt2d Pp)
/*< orthogonal step forward  >*/
{
    pt2d   Ro;
    float  lo;
    double sina,cosa,tana;
    int    ss;

    lo = 0.5 * DST2d(Pm,Pp);

    if(Pp.x!=Pm.x) {
	tana = (Pp.z-Pm.z) / (Pp.x-Pm.x);
	ss   = SF_SIG(tana);
	
	tana = SF_ABS(tana);	
	sina = tana/sqrt(1+tana*tana);
	cosa =    1/sqrt(1+tana*tana);
    } else {
	ss = 1;

	cosa = 0;
	sina = 1;
    }

    Ro.x = Po.x - ss * lo * sina;
    Ro.z = Po.z +      lo * cosa;
    Ro.v=hwtgetv(Ro);
    
    return(Ro);
}
