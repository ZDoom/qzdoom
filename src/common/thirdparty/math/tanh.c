/*							tanh.c
 *
 *	Hyperbolic tangent
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, tanh();
 *
 * y = tanh( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns hyperbolic tangent of argument in the range MINLOG to
 * MAXLOG.
 *
 * A rational function is used for |x| < 0.625.  The form
 * x + x**3 P(x)/Q(x) of Cody _& Waite is employed.
 * Otherwise,
 *    tanh(x) = sinh(x)/cosh(x) = 1  -  2/(exp(2x) + 1).
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -2,2        50000       3.3e-17     6.4e-18
 *    IEEE      -2,2        30000       2.5e-16     5.8e-17
 *
 */

/*
Cephes Math Library Release 2.8:  June, 2000
Copyright 1984, 1995, 2000 by Stephen L. Moshier

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the <ORGANIZATION> nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "mconf.h"

#ifdef UNK
static double P[] = {
-9.64399179425052238628E-1,
-9.92877231001918586564E1,
-1.61468768441708447952E3
};
static double Q[] = {
/* 1.00000000000000000000E0,*/
 1.12811678491632931402E2,
 2.23548839060100448583E3,
 4.84406305325125486048E3
};
#endif
#ifdef DEC
static unsigned short P[] = {
0140166,0161335,0053753,0075126,
0141706,0111520,0070463,0040552,
0142711,0153001,0101300,0025430
};
static unsigned short Q[] = {
/*0040200,0000000,0000000,0000000,*/
0041741,0117624,0051300,0156060,
0043013,0133720,0071251,0127717,
0043227,0060201,0021020,0020136
};
#endif

#ifdef IBMPC
static unsigned short P[] = {
0x6f4b,0xaafd,0xdc5b,0xbfee,
0x682d,0x0e26,0xd26a,0xc058,
0x0563,0x3058,0x3ac0,0xc099
};
static unsigned short Q[] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
0x1b86,0x8a58,0x33f2,0x405c,
0x35fa,0x0e55,0x76fa,0x40a1,
0x040c,0x2442,0xec10,0x40b2
};
#endif

#ifdef MIEEE
static unsigned short P[] = {
0xbfee,0xdc5b,0xaafd,0x6f4b,
0xc058,0xd26a,0x0e26,0x682d,
0xc099,0x3ac0,0x3058,0x0563
};
static unsigned short Q[] = {
0x405c,0x33f2,0x8a58,0x1b86,
0x40a1,0x76fa,0x0e55,0x35fa,
0x40b2,0xec10,0x2442,0x040c
};
#endif

#ifdef ANSIPROT
extern double fabs ( double );
extern double c_exp ( double );
extern double polevl ( double, void *, int );
extern double p1evl ( double, void *, int );
#else
double fabs(), c_exp(), polevl(), p1evl();
#endif
extern double MAXLOG;

double c_tanh(x)
double x;
{
double s, z;

#ifdef MINUSZERO
if( x == 0.0 )
	return(x);
#endif
z = fabs(x);
if( z > 0.5 * MAXLOG )
	{
	if( x > 0 )
		return( 1.0 );
	else
		return( -1.0 );
	}
if( z >= 0.625 )
	{
	s = c_exp(2.0*z);
	z =  1.0  - 2.0/(s + 1.0);
	if( x < 0 )
		z = -z;
	}
else
	{
	if( x == 0.0 )
	  return(x);
	s = x * x;
	z = polevl( s, P, 2 )/p1evl(s, Q, 3);
	z = x * s * z;
	z = x + z;
	}
return( z );
}
