/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Huang, Johnathan( Intel Lab )
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "mmath.h"
#include "dct.h"

void dct_init( Dct_16s **phDct, int startIdx, int endIdx, int len, int Q )
{
	short *ptr;
	int   n, k, numOut;
	int   S;

	// XXX: why right shift (Q - 1) rather than Q here: avoid overflow, will compensate after DCT calculation
	S = ( 1 << Q ) - 1;

	*phDct = NULL;
	*phDct = (Dct_16s*)calloc( sizeof(Dct_16s), 1 );
	if ( *phDct == NULL )
	{
		return;
	}

	numOut             = endIdx - startIdx + 1;

	(*phDct)->startIdx = startIdx;
	(*phDct)->endIdx   = endIdx;
	(*phDct)->len      = len;
	(*phDct)->Q        = Q;
	(*phDct)->w        = (short*)malloc( numOut * len * sizeof(short) );
	(*phDct)->inBuf    = (int*)malloc( len * sizeof(int) );

	ptr                = (*phDct)->w;
	for ( k = startIdx; k <= endIdx; k++ )
	{
		// DCT-II
		for ( n = 1; n <= len; n++ )
		{
			*ptr = (short)round( S * cos( PI * k * (n - 0.5) / len ) );
			// printf( "k = %d, n = %d, %d\n", k, n, *ptr );
			ptr++;
		}
	}

	return;
}


void dct_calc( Dct_16s *pDct, int *inBuf, int *outBuf )
{
	short    *ptr = NULL;
	int      minLz, lz;
	int      n, k;
	int64_t  out;

	minLz = 32;
	for ( n = 0; n < pDct->len; n++ )
	{
		lz = clz( inBuf[n] );
		if ( lz < minLz )
		{
			minLz = lz;
		}
	}
	for ( n = 0; n < pDct->len; n++ )
	{
		if ( minLz < 17 )
		{
			pDct->inBuf[n] = inBuf[n] >> ( 17 - minLz );
		}
		else
		{
			pDct->inBuf[n] = inBuf[n];
		}
	}

	ptr = pDct->w;
	for ( k = pDct->startIdx; k <= pDct->endIdx; k++ )
	{
		out = 0;
		for ( n = 0; n < pDct->len; n++ )
		{
			out += pDct->inBuf[n] * (*ptr);
			// printf( "%d, %d\n", inBuf[n], *ptr );
			ptr++;
		}
		// XXX: why right shift 5 here: to cast to short type, need left shift 16, but while calc DCT, only Q - 1
		//      left shift, so to compensate, need lesft shift 15, we made assumption DCT coefficient as Q10 here
		//      so left shift ( 15 - 10 ) = 5 here
		// out = ( out >> 5 ); // scale to Q10

		if ( minLz < 17 )
		{
			int offset = pDct->Q - (17 - minLz);
			if ( offset > 0 )
			{
				out = out >> offset;
			}
			else if ( offset < 0 )
			{
				out = out << (-offset);
			}
		}
		else
		{
			out = out >> pDct->Q;
		}

		if ( out > MAX_INT32S )
		{
			out = MAX_INT32S;
		}
		else if ( out < -MAX_INT32S - 1 )
		{
			out = -MAX_INT32S - 1;
		}

		*outBuf = (int)out;
		outBuf++;
	}

	return;
}

void dct_free( Dct_16s **phDct )
{
	free( (*phDct)->w );
	free( (*phDct)->inBuf );
	free( *phDct );

	*phDct = NULL;

	return;
}
