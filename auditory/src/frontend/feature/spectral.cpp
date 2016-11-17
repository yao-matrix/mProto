#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"
#include "spectral.h"

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

AUD_Double freqBand[5][2] = { {300, 627}, {628, 1060}, {1061, 1633}, {1634, 2393}, {2394, 3400} };


void specfeat_init( SpecFeat **phSpecFeat, AUD_Int32s fftLen, AUD_Float fs )
{
	SpecFeat   *pSpecFeat = NULL;
	AUD_Int32s i;

	pSpecFeat = (SpecFeat*)calloc( sizeof(SpecFeat), 1 );
	if ( pSpecFeat == NULL )
	{
		*phSpecFeat = NULL;
		return;
	}

	for ( i = 0; i < 3; i++ )
	{
		pSpecFeat->pState[i] = (AUD_Int32s*)calloc( SPECFEAT_LEN * sizeof(AUD_Int32s), 1 );
		AUD_ASSERT( pSpecFeat->pState[i] );
	}

	pSpecFeat->fftLen = fftLen;
	pSpecFeat->fs     = fs;

	*phSpecFeat       = pSpecFeat;

	return;
}

void specfeat_calc( SpecFeat *pSpecFeat, AUD_Int32s *pFFT, AUD_Int32s *pOut )
{
	AUD_ASSERT( pSpecFeat && pFFT && pOut );

	AUD_Int32s i, j, k;

	AUD_Double sc[5];
	AUD_Double weight[5];

	AUD_Double sum[5];
	AUD_Double renyi[5];

	AUD_Double alpha = 3.;

#if 0
	AUDLOG( "input:\n" );
	for ( i = 0; i < FRAME_LEN / 2; i++ )
	{
		AUDLOG( "%d, ", pFFT[i]  );
	}
	AUDLOG( "\n" );
#endif

	for ( i = 2; i >= 1; i-- )
	{
		for ( j = 0; j < SPECFEAT_LEN; j++ )
		{
			pSpecFeat->pState[i][j] = pSpecFeat->pState[i - 1][j];
		}
	}

	k = 0;
	AUD_Double freqStep = pSpecFeat->fs / pSpecFeat->fftLen;
	AUD_Double fft;
	for ( i = 0; i < 5; i++ )
	{
		sc[i]     = 0.;
		weight[i] = 0.;
		sum[i]    = 0.;
		renyi[i]  = 0.;
		while ( k * freqStep <= freqBand[i][1] )
		{
			if ( k * freqStep < freqBand[i][0] )
			{
				k++;
				continue;
			}
			else
			{
				fft = pFFT[k] / 65536.;
				weight[i] += fft * fft;
				sc[i]     += k * freqStep * fft * fft;
				sum[i]    += fft;
				renyi[i]  += pow( fft, alpha );
				k++;
			}
		}
	}

	for ( i = 0; i < 5; i++ )
	{
		// AUDLOG( "sc: %.2f, weight: %.2f, renyi: %.2f, sum:%.2f\n", sc[i], weight[i], renyi[i], sum[i] );
		sc[i]    = sc[i] / weight[i];
		renyi[i] = ( log( pow( sum[i], alpha ) ) - log( renyi[i] ) ) / ( alpha - 1 ) / log( 2 );
		pSpecFeat->pState[0][i]     = (AUD_Int32s)round( sc[i] );
		pSpecFeat->pState[0][i + 5] = (AUD_Int32s)round( renyi[i] * 32767. );
		// AUDLOG( "sc: %.2f, renyi: %.2f, \n", sc[i] , renyi[i] );
	}

#if 0
	AUDLOG( "out: \n" );
	for ( i = 0; i < SPECFEAT_LEN; i++ )
	{
		AUDLOG( "%d, ", pSpecFeat->pState[0][i]  );
	}
	AUDLOG( "\n\n" );
#endif

	for ( i = 0; i < SPECFEAT_LEN; i++ )
	{
		pOut[i] = pSpecFeat->pState[2][i];
	}

	return;
}

void specfeat_free( SpecFeat **phSpecFeat )
{
	SpecFeat   *pSpecFeat = *phSpecFeat;
	AUD_Int32s i;

	for ( i = 0; i < 5; i++ )
	{
		free( pSpecFeat->pState[i] );
	}
	free( pSpecFeat );

	*phSpecFeat = NULL;

	return;
}
