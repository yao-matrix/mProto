#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"
#include "mfcc16s.h"

#include "AudioDef.h"
#include "AudioUtil.h"

void mfccrasta_init( Mfcc_rasta **phMrasta, AUD_Int32s mfccLen )
{
	Mfcc_rasta *pMrasta = NULL;
	pMrasta = (Mfcc_rasta*)calloc( sizeof(Mfcc_rasta), 1 );
	if ( pMrasta == NULL )
	{
		*phMrasta = NULL;
		return;
	}

	AUD_Int32s i;
	for ( i = 0; i < RASTA_BUFLEN; i++ )
	{
		pMrasta->pXState[i] = (AUD_Int32s*)calloc( mfccLen * sizeof(AUD_Int32s), 1 );
		AUD_ASSERT( pMrasta->pXState[i] );
	}
	pMrasta->ys = (AUD_Int32s*)calloc( mfccLen * sizeof(AUD_Int32s), 1 );
	AUD_ASSERT( pMrasta->ys );

	pMrasta->b[0] = -0.2;
	pMrasta->b[1] = -0.1;
	pMrasta->b[2] = 0.;
	pMrasta->b[3] = -pMrasta->b[1];
	pMrasta->b[4] = -pMrasta->b[0];

	pMrasta->a    = 0.98;

	pMrasta->mfccLen = mfccLen;

	*phMrasta = pMrasta;

	return;
}

void mfccrasta_refresh( Mfcc_rasta *pMrasta )
{
	AUD_Int32s i;

	for ( i = 0; i < RASTA_BUFLEN; i++ )
	{
		memset( pMrasta->pXState[i], 0, pMrasta->mfccLen * sizeof(AUD_Int32s) );
	}
	memset( pMrasta->ys, 0, pMrasta->mfccLen * sizeof(AUD_Int32s) );

	return;
}


void mfccrasta_calc( Mfcc_rasta *pMrasta, AUD_Int32s *pIn, AUD_Int32s *pOut )
{
	AUD_Int32s i, j;
	AUD_Double tmp = 0.;

	for ( i = RASTA_BUFLEN - 1; i >= 1; i-- )
	{
		for ( j = 0; j < pMrasta->mfccLen; j++ )
		{
			pMrasta->pXState[i][j] = pMrasta->pXState[i - 1][j];
		}
	}

	for ( j = 0; j < pMrasta->mfccLen; j++ )
	{
		pMrasta->pXState[0][j] = pIn[j];
	}

	for ( i = 0; i < pMrasta->mfccLen; i++ )
	{
		tmp = 0.;
		for ( j = 0; j < RATSA_ORDER; j++ )
		{
			tmp += pMrasta->pXState[RASTA_BUFLEN - 1 - j][i] * pMrasta->b[j];
		}
		tmp += pMrasta->ys[i] * pMrasta->a;

		pOut[i] = (AUD_Int32s)round( tmp );
	}

	for ( i = 0; i < pMrasta->mfccLen; i++ )
	{
		pMrasta->ys[i] = pOut[i];
	}

	return;
}

void mfccrasta_free( Mfcc_rasta **phMrasta )
{
	Mfcc_rasta *pMrasta = *phMrasta;
	AUD_Int32s i        = 0;

	for ( i = 0; i < RASTA_BUFLEN; i++ )
	{
		free( pMrasta->pXState[i] );
	}
	free( pMrasta->ys );

	free( pMrasta );

	*phMrasta = NULL;

	return;
}


void mfccd_init( Mfcc_delta **phMd, AUD_Int32s mfccLen )
{
	Mfcc_delta *pMd = NULL;
	AUD_Int32s i;

	pMd = (Mfcc_delta*)calloc( sizeof(Mfcc_delta), 1 );
	if ( pMd == NULL )
	{
		*phMd = NULL;
		return;
	}

	for ( i = 0; i < DELTA_BUFLEN; i++ )
	{
		pMd->pState[i] = (AUD_Int32s*)calloc( mfccLen * sizeof(AUD_Int32s), 1 );
		AUD_ASSERT( pMd->pState[i] );
	}

	pMd->c[0] = -0.2;
	pMd->c[1] = -0.1;
	pMd->c[2] = 0.;
	pMd->c[3] = -pMd->c[1];
	pMd->c[4] = -pMd->c[0];

	pMd->mfccLen = mfccLen;

	*phMd = pMd;

	return;
}

void mfccd_refresh( Mfcc_delta *pMd )
{
	AUD_Int32s i, j;

	for ( i = 0; i < DELTA_BUFLEN; i++ )
	{
		for ( j = 0; j < pMd->mfccLen; j++ )
		{
			pMd->pState[i][j] = 0;
		}
	}

	return;
}


void mfccd_calc( Mfcc_delta *pMd, AUD_Int32s *pIn, AUD_Int32s *pOut )
{
	AUD_Int32s i, j;
	AUD_Double tmp = 0.;

	for ( i = DELTA_BUFLEN - 1; i >= 1; i-- )
	{
		for ( j = 0; j < pMd->mfccLen; j++ )
		{
			pMd->pState[i][j] = pMd->pState[i - 1][j];
		}
	}
	for ( j = 0; j < pMd->mfccLen; j++ )
	{
		pMd->pState[0][j] = pIn[j];
	}

	for ( i = 0; i < pMd->mfccLen; i++ )
	{
		tmp = 0.;
		for ( j = 0; j < DELTA_ORDER; j++ )
		{
			tmp += pMd->pState[DELTA_BUFLEN - 1 - j][i] * pMd->c[j];
		}

		pOut[i] = (AUD_Int32s)round( tmp );
	}

	return;
}

void mfccd_free( Mfcc_delta **phMd )
{
	Mfcc_delta *pMd = *phMd;
	AUD_Int32s i;

	for ( i = 0; i < DELTA_BUFLEN; i++ )
	{
		free( pMd->pState[i] );
	}

	free( pMd );

	*phMd = NULL;

	return;
}

void mfccdd_init( Mfcc_deltadelta **phMdd, AUD_Int32s mfccLen )
{
	Mfcc_deltadelta *pMdd = NULL;
	AUD_Int32s      i;

	pMdd = (Mfcc_deltadelta*)calloc( sizeof(Mfcc_deltadelta), 1 );
	if ( pMdd == NULL )
	{
		*phMdd = NULL;
		return;
	}

	for ( i = 0; i < DDELTA_BUFLEN; i++ )
	{
		pMdd->pState[i] = (AUD_Int32s*)calloc( mfccLen * sizeof(AUD_Int32s), 1 );
		AUD_ASSERT( pMdd->pState[i] );
	}

	pMdd->c[0] = 0.04;
	pMdd->c[1] = 0.04;
	pMdd->c[2] = 0.01;
	pMdd->c[3] = -0.04;
	pMdd->c[4] = -0.1;
	pMdd->c[5] = -0.04;
	pMdd->c[6] = 0.01;
	pMdd->c[7] = 0.04;
	pMdd->c[8] = 0.04;

	pMdd->mfccLen = mfccLen;

	*phMdd = pMdd;

	return;
}

void mfccdd_refresh( Mfcc_deltadelta *pMdd )
{
	AUD_Int32s i, j;

	for ( i = 0; i < DDELTA_BUFLEN; i++ )
	{
		for ( j = 0; j < pMdd->mfccLen; j++ )
		{
			pMdd->pState[i][j] = 0;
		}
	}

	return;
}


void mfccdd_calc( Mfcc_deltadelta *pMdd, AUD_Int32s *pIn, AUD_Int32s *pOut )
{
	AUD_Int32s i, j;
	AUD_Double tmp = 0.;

	for ( i = DDELTA_BUFLEN - 1; i >= 1; i-- )
	{
		for ( j = 0; j < pMdd->mfccLen; j++ )
		{
			pMdd->pState[i][j] = pMdd->pState[i - 1][j];
		}
	}
	for ( j = 0; j < pMdd->mfccLen; j++ )
	{
		pMdd->pState[0][j] = pIn[j];
	}

	for ( i = 0; i < pMdd->mfccLen; i++ )
	{
		tmp = 0.;
		for ( j = 0; j < DDELTA_ORDER; j++ )
		{
			tmp += pMdd->pState[j][i] * pMdd->c[j];
		}

		pOut[i] = (AUD_Int32s)round( tmp );
	}

	return;
}

void mfccdd_free( Mfcc_deltadelta **phMdd )
{
	Mfcc_deltadelta *pMdd = *phMdd;
	AUD_Int32s      i;

	for ( i = 0; i < DDELTA_BUFLEN; i++ )
	{
		free( pMdd->pState[i] );
	}

	free( pMdd );

	*phMdd = NULL;

	return;
}

void mfccfd_init( Mfcc_fdelta **phMfd, AUD_Int32s mfccLen )
{
	Mfcc_fdelta *pMfd = NULL;

	pMfd = (Mfcc_fdelta*)calloc( sizeof(Mfcc_fdelta), 1 );
	if ( pMfd == NULL )
	{
		*phMfd = NULL;
		return;
	}

	pMfd->c0 = (AUD_Int32s)(0.5 * (1 << 15));
	pMfd->c1 = (AUD_Int32s)(-1 * (1 << 15));
	pMfd->c2 = (AUD_Int32s)(0.5 * (1 << 15));

	pMfd->mfccLen = mfccLen;

	*phMfd = pMfd;

	return;
}

/* opportunity to replace with vectorized operations */
void mfccfd_calc( Mfcc_fdelta *pMfd, AUD_Int32s *pIn, AUD_Int32s *pOut )
{
	AUD_Int32s i;

	pOut[0] = pOut[1] = 0;
	for ( i = 2; i < pMfd->mfccLen; i++ )
	{
		pOut[i] = (AUD_Int32s)( 0.5 * pIn[i] - pIn[i - 1] + 0.5 * pIn[i - 2] );
	}

	return;
}

void mfccfd_free( Mfcc_fdelta **phMfd )
{
	Mfcc_fdelta *pMfd = *phMfd;

	free( pMfd );

	*phMfd = NULL;

	return;
}

void mfcc_init( Mfcc **phMfcc, AUD_Int32s winSize, AUD_Float fs,
                AUD_Float fl, AUD_Float fh, AUD_Int32s fbLen, AUD_Float melMul, AUD_Float melDiv, AUD_Int32s mfccLen, AUD_AmpCompress compressType )
{
	Mfcc       *pMfcc = NULL;
	AUD_Int32s fftOrder, i;

	pMfcc = (Mfcc*)calloc( sizeof(Mfcc), 1 );
	if ( pMfcc == NULL )
	{
		*phMfcc = NULL;
		return;
	}
	*phMfcc = pMfcc;

	pMfcc->fftLen       = winSize / 2;
	pMfcc->fbLen        = fbLen;
	pMfcc->mfccLen      = mfccLen;
	pMfcc->compressType = compressType;

	melfb_init( &(pMfcc->pFb), &fftOrder, winSize, fs, fl, fh, fbLen, melMul, melDiv );

	pMfcc->fbBuffer = (AUD_Int32s*)calloc( fbLen * sizeof(AUD_Int32s), 1 );
	if ( pMfcc->fbBuffer == NULL )
	{
		free( pMfcc );
		*phMfcc = NULL;
		return;
	}

	dct_init( &(pMfcc->pDct), 1, mfccLen, fbLen, 15 );

	for ( i = 0; i < RAW_BUFLEN; i++ )
	{
		pMfcc->pState[i] = (AUD_Int32s*)calloc( mfccLen * sizeof(AUD_Int32s), 1 );
		AUD_ASSERT( pMfcc->pState[i] );
	}

	return;
}

void mfcc_calc( Mfcc *pMfcc, AUD_Int32s *Xabs, AUD_Int32s *mfcc )
{
	AUD_Int32s lz, minLz;
	AUD_Int32s i, j, k;

	/* find the number of leading zeros in the largest number */
	minLz = 32;
	for ( k = 0; k < pMfcc->fftLen; k++ )
	{
		lz = clz( Xabs[k] );
		if ( lz < minLz )
		{
			minLz = lz;
		}
	}
	/* scale down the to have at least 15 leading zeros to prevent saturation in next step */
	if ( minLz < 15 )
	{
		for ( k = 0; k < pMfcc->fftLen; k++ )
		{
			Xabs[k] = Xabs[k] >> (15 - minLz);
		}
	}

	/* filter bank calculation */
	melfb_calc( pMfcc->pFb, Xabs, pMfcc->fbBuffer );

	if ( pMfcc->compressType == AUD_AMPCOMPRESS_LOG )
	{
		for ( k = 0; k < pMfcc->fbLen; k++ )
		{
			/* make sure we have no zeros in the ln() calulation */
			if ( pMfcc->fbBuffer[k] <= 0 )
			{
				pMfcc->fbBuffer[k] = 1;
			}

			pMfcc->fbBuffer[k] = fp_ln( pMfcc->fbBuffer[k] );
		}
	}
	else if ( pMfcc->compressType == AUD_AMPCOMPRESS_ROOT )
	{
		for ( k = 0; k < pMfcc->fbLen; k++ )
		{
			// in noisy environment, a bigger root lead to better feature discriminative capability
			// refer to "Improving the noise-robustness of mel-frequency cepstral coefficients for speech recognition"
			if ( pMfcc->fbBuffer[k] <= 0 )
			{
				pMfcc->fbBuffer[k] = 0;
			}
			else
			{
				pMfcc->fbBuffer[k] = (AUD_Int32s)round( pow( pMfcc->fbBuffer[k] / 65536.0, 0.14 ) * 65536.0 );
			}
		}
	}
	else
	{
		AUD_ASSERT( 0 );
	}

	/* DCT calculation */
	dct_calc( pMfcc->pDct, pMfcc->fbBuffer, mfcc );

	/* buffering */
	for ( i = RAW_BUFLEN - 1; i >= 1; i-- )
	{
		for ( j = 0; j < pMfcc->mfccLen; j++ )
		{
			pMfcc->pState[i][j] = pMfcc->pState[i - 1][j];
		}
	}
	for ( j = 0; j < pMfcc->mfccLen; j++ )
	{
		pMfcc->pState[0][j] = mfcc[j];
	}

	return;
}

void mfcc_get( Mfcc *pMfcc, AUD_Int32s *pOut )
{
	AUD_Int32s i;

	for ( i = 0; i < pMfcc->mfccLen; i++ )
	{
		pOut[i] = pMfcc->pState[RAW_BUFLEN - 1][i];
	}
	return;
}

void mfcc_free( Mfcc **phMfcc )
{
	Mfcc       *pMfcc = *phMfcc;
	AUD_Int32s i;

	free( pMfcc->fbBuffer );
	melfb_free( &(pMfcc->pFb) );
	dct_free( &(pMfcc->pDct) );

	for ( i = 0; i < RAW_BUFLEN; i++ )
	{
		free( pMfcc->pState[i] );
	}

	free( pMfcc );
	*phMfcc = NULL;

	return;
}

static AUD_Int32s** Alloc2_32s( AUD_Int32s row, AUD_Int32s col )
{
	AUD_Int32s **ptr, *ptr1;
	AUD_Int32s i;
	AUD_Int32s col1 = (col + 7) & (~7);

	ptr = (AUD_Int32s**)malloc( row * sizeof(AUD_Int32s*) + (row * col1 + 7) * sizeof(AUD_Int32s) );
	if ( ptr == NULL )
	{
		return NULL;
	}

	ptr1 = (AUD_Int32s*)(ptr + row);
	i = ((AUD_Int8u*)ptr1 - (AUD_Int8u*)0) & 31;
	if ( i )
	{
		ptr1 = (AUD_Int32s*)((AUD_Int8u*)ptr1 + 32 - i);
	}

	for ( i = 0; i < row; ptr1 += col1, i++ )
	{
		ptr[i] = ptr1;
	}

	return ptr;
}

void melfb_init( MelfbState **pFBank, AUD_Int32s *pFFTLen, AUD_Int32s winSize, AUD_Float sampFreq, AUD_Float lowFreq, AUD_Float highFreq, AUD_Int32s nFilter, AUD_Float melMul, AUD_Float melDiv )
{
	MelfbState     *pFBankState;
	AUD_Int32s     yk0, yk1, i, j, k;
	AUD_Int32s     lenFFT, scale, size;
	AUD_Int32s     **pCoeff1Q14;
	AUD_Int32s     **pCoeff2Q14;
	AUD_Float      *pCentersMel;
	AUD_Float      melHL, melLow, melHigh, ck, ck1;
	AUD_Double     tmp;


	*pFFTLen = 1;
	lenFFT   = 2;
	while ( lenFFT < winSize )
	{
		(*pFFTLen) += 1;
		lenFFT *= 2;
	}

	melLow   = (AUD_Float)(melMul * log( 1. + lowFreq / melDiv ));
	melHigh  = (AUD_Float)(melMul * log( 1. + highFreq / melDiv ));

	melHL = melHigh - melLow;

	pFBankState = (MelfbState*)calloc( sizeof(MelfbState), 1 );
	AUD_ASSERT( pFBankState );

	*pFBank               = pFBankState;
	pFBankState->FFTOrder = (*pFFTLen);
	pFBankState->FFTLen   = lenFFT;
	pFBankState->nFilter  = nFilter;
	pFBankState->melMul   = melMul;
	pFBankState->melDiv   = melDiv;
	pFBankState->Q        = 14;
	scale                 = 1 << pFBankState->Q;

	pFBankState->pCentersFFT   = (AUD_Int32s*)malloc( sizeof(AUD_Int32s) * (nFilter + 2) );
	pCentersMel                = (AUD_Float*)malloc( sizeof(AUD_Float) * (nFilter + 2) );
	pFBankState->pWeightsLen   = (AUD_Int32s*)malloc( sizeof(AUD_Int32s) * nFilter );
	pFBankState->pWeightsQ14   = (AUD_Int32s**)malloc( sizeof(AUD_Int32s*) * nFilter );

	pCoeff1Q14 = Alloc2_32s( (nFilter + 2), (lenFFT / 2) );
	pCoeff2Q14 = Alloc2_32s( (nFilter + 2), (lenFFT / 2) );

	AUD_ASSERT( pFBankState->pCentersFFT && pCentersMel && pFBankState->pWeightsLen && pFBankState->pWeightsQ14 && pCoeff1Q14 && pCoeff2Q14 );

	for ( k = 0; k <= nFilter + 1; k++ )
	{
		for ( i = 0; i < lenFFT / 2; i++ )
		{
			pCoeff1Q14[k][i] = 0;
			pCoeff2Q14[k][i] = 0;
		}
	}

	/* calculate the center of each filter bank */
	for ( k = 0; k <= nFilter + 1; k++ )
	{
		pCentersMel[k] = melLow + ( k * melHL ) / (nFilter + 1);

		ck = melDiv * ( exp( pCentersMel[k] / melMul ) - 1. );

		pFBankState->pCentersFFT[k] = (AUD_Int32s)round( lenFFT * ck / sampFreq );

		pCentersMel[k] = melMul * log( 1. + pFBankState->pCentersFFT[k] * sampFreq / lenFFT / melDiv );
	}

	for ( k = 1; k <= pFBankState->nFilter; k++ )
	{
		ck  = pCentersMel[k] - pCentersMel[k - 1];
		ck1 = pCentersMel[k - 1];
		yk0 = pFBankState->pCentersFFT[k - 1];
		yk1 = pFBankState->pCentersFFT[k];

		for ( i = yk0 + 1; i <= yk1; i++ )
		{
			tmp = round( (AUD_Double)( melMul * log( 1. + i * sampFreq / lenFFT / melDiv ) - ck1 ) / ck * scale );
			if ( tmp > scale )
			{
				tmp = scale;
			}
			else if ( tmp < 0 )
			{
				tmp = 0;
			}
			pCoeff1Q14[k - 1][i - yk0]= (AUD_Int32s)tmp;
		}

		ck  = pCentersMel[k + 1] - pCentersMel[k];
		ck1 = pCentersMel[k + 1];
		yk1 = pFBankState->pCentersFFT[k + 1];
		yk0 = pFBankState->pCentersFFT[k] + 1;
		for ( i = yk0; i < yk1; i++ )
		{
			tmp = round( (AUD_Double)( ck1 - melMul * log( 1. + i * sampFreq / lenFFT / melDiv ) ) / ck * scale );
			if ( tmp > scale )
			{
				tmp = scale;
			}
			else if ( tmp < 0 )
			{
				tmp = 0;
			}
			pCoeff2Q14[k - 1][i - yk0] = (AUD_Int32s)tmp;
		}
	}

	for ( k = 1; k <= pFBankState->nFilter; k++ )
	{
		pFBankState->pWeightsLen[k - 1] = pFBankState->pCentersFFT[k + 1] - pFBankState->pCentersFFT[k - 1] + 1;
		pFBankState->pWeightsQ14[k - 1] = (AUD_Int32s*)malloc( pFBankState->pWeightsLen[k - 1] * sizeof(AUD_Int32s) );
		AUD_ASSERT( pFBankState->pWeightsQ14[k - 1] );

		size = pFBankState->pCentersFFT[k] - pFBankState->pCentersFFT[k - 1];
		for ( i = 0; i <= size; i++ )
		{
			pFBankState->pWeightsQ14[k - 1][i] = pCoeff1Q14[k - 1][i];
		}
		size = pFBankState->pCentersFFT[k + 1] - pFBankState->pCentersFFT[k];
		for ( j = 0; j < size; i++, j++ )
		{
			pFBankState->pWeightsQ14[k - 1][i] = pCoeff2Q14[k - 1][j];
		}
	}

	if ( pCentersMel )
	{
		free( pCentersMel );
		pCentersMel = NULL;
	}
	if ( pCoeff1Q14 )
	{
		free( pCoeff1Q14 );
		pCoeff1Q14 = NULL;
	}
	if ( pCoeff2Q14 )
	{
		free( pCoeff2Q14 );
		pCoeff2Q14 = NULL;
	}

#if 0
	// print out mel filter bank
	for ( k = 1; k <= pFBankState->nFilter; k++ )
	{
		AUDLOG( "[%d, %d]:", pFBankState->pCentersFFT[k - 1], pFBankState->pCentersFFT[k + 1] );
		for ( i = 0; i < pFBankState->pWeightsLen[k - 1]; i++ )
		{
			AUDLOG( "%.3f, ", pFBankState->pWeightsQ14[k - 1][i] / pow( 2., 14 ) );
		}
		AUDLOG( "\n" );
	}
#endif

	return;
}

void melfb_calc( MelfbState *pFb, AUD_Int32s *X, AUD_Int32s *fb )
{
	AUD_Int32s k, stIdx, len;

	for ( k = 1; k <= pFb->nFilter; k++ )
	{
		len       = pFb->pCentersFFT[k + 1] - pFb->pCentersFFT[k - 1] + 1;
		stIdx     = pFb->pCentersFFT[k - 1];
		fb[k - 1] = dotproduct( pFb->pWeightsQ14[k - 1], &X[stIdx], len, pFb->Q );
	}

	return;
}

void melfb_free( MelfbState **phFb )
{
	MelfbState *pFb = *phFb;
	AUD_Int32s i;

	for ( i = 0; i < pFb->nFilter; i++ )
	{
		free( pFb->pWeightsQ14[i] );
	}
	free( pFb->pWeightsQ14 );
	free( pFb->pCentersFFT );
	free( pFb->pWeightsLen );
	free( pFb );

	*phFb = NULL;

	return;
}
