/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Huang, Johnathan( Intel Lab )
 *   Author: Yao, Matrix( matrix.yao@AUD_Int32sel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <memory.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "mmath.h"

AUD_Int32s mean( AUD_Int16s *x, AUD_Int32s len )
{
	AUD_Int32s i, s;

	s = 0;

	for ( i = 0; i < len; i++ )
	{
		s += x[i];
	}

	s /= len;

	return s;
}


AUD_Int32u variance( AUD_Int16s *x, AUD_Int32s len, AUD_Int32s shift )
{
	AUD_Int32s i, s, t;
	AUD_Int32u y;

	s = mean( x, len );

	y = 0;

	for ( i = 0; i < len; i++ )
	{
		t = x[i] - s;

		y += (t * t) >> shift;
	}

	y /= len;

	return y;
}


AUD_Int32s dotproduct( AUD_Int32s *x, AUD_Int32s *y, AUD_Int32s len, AUD_Int32s shift )
{
	AUD_Int32s  i;
	AUD_Int32s  sum1 = 0;
	AUD_Int64s  sum  = 0;

	for ( i = 0; i < len; i++ )
	{
		sum += (AUD_Int64s)x[i] * ((AUD_Int64s)y[i]);
	}

	sum1 = (AUD_Int32s)( sum >> shift );

	return sum1;
}


/* count leading zeros of a number. make it positive */
AUD_Int32s clz( AUD_Int32s val )
{
	AUD_Int32s i, j, uval;

	uval = val;

	if ( uval < 0 )
	{
		uval = -uval;
	}

	for ( i = 31, j = 0; i >= 0; i--, j++ )
	{
		if ( (uval >> i) == 1 )
		{
			break;
		}
	}

	return j;
}


#define iter1( N ) \
    tr = root + (1 << (N)); \
    if ( n >= tr << (N) )   \
    {   n -= tr << (N);   \
        root |= 2 << (N); \
    }

AUD_Int32s isqrt( AUD_Int32s n )
{
	AUD_Int32s root = 0, tr;
	iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
	iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8);
	iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4);
	iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0);

	return root >> 1;
}


// fp_ln( x ) = log( x / 65536.0 ) * 65536.0
AUD_Int32s fp_ln( AUD_Int32s val )
{
#define base 16

	AUD_Int32s fracv, intv, y, ysq, fracr, bitpos;

	/*

	fracv    -    initial fraction part from "val"

	intv     -    initial integer part from "val"

	y        -    (fracv-1)/(fracv+1)

	ysq      -    y * y

	fracr    -    ln( fracv )

	bitpos   -    integer part of log2( val )

	*/


	// const AUD_Int32s ILN2 = 94548;        /* 1/ln(2) with 2^16 as base*/

	const AUD_Int32s ILOG2E = 45426;    /* 1/log2(e) with 2^16 as base */


	const AUD_Int32s ln_denoms[] = {

		(1 << base) / 1,

		(1 << base) / 3,

		(1 << base) / 5,

		(1 << base) / 7,

		(1 << base) / 9,

		(1 << base) / 11,

		(1 << base) / 13,

		(1 << base) / 15,

		(1 << base) / 17,

		(1 << base) / 19,

		(1 << base) / 21,

	};


	/* compute fracv and AUD_Int32sv */
	bitpos = 15 - clz( val );

	if ( bitpos >= 0 )
	{
		++bitpos;

		fracv = val >> bitpos;
	}
	else if ( bitpos < 0 )
	{
		/* fracr = val / 2^-(bitpos) */
		++bitpos;

		fracv = val << (-bitpos);
	}


	// bitpos is the integer part of ln(val), but in log2, so we convert

	// ln(val) = log2(val) / log2(e)
	intv = bitpos * ILOG2E;


	// y = (ln_fraction_value−1)/(ln_fraction_value+1)
	y = ((AUD_Int64s)(fracv - (1 << base)) << base) / (fracv + (1 << base));


	ysq = (y * y) >> base;

	fracr = ln_denoms[10];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[9];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[8];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[7];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[6];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[5];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[4];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[3];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[2];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[1];

	fracr = (((AUD_Int64s)fracr * ysq)>>base) + ln_denoms[0];

	fracr = ((AUD_Int64s)fracr * (y << 1)) >> base;

	return intv + fracr;
}


/* algorithm from sphinx  /SphinxTrain/programs/bw/main.c */
AUD_Double logadd( AUD_Double x, AUD_Double y )
{
	AUD_Double z;

	if ( x < y )
	{
		return logadd( y, x );
	}

	if ( y <= LOG_ZERO )
	{
		return x;
	}
	else
	{
		z = exp( y - x );
		return x + log( 1.0 + z );
	}
}

