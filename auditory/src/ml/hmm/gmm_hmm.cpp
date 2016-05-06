/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"
#include "AudioConfig.h"

#include "math/mmath.h"
#include "misc/misc.h"

#include "../gmm/gmm.h"
#include "util.h"

typedef struct
{
	AUD_Int8s  arHmmName[256];
	AUD_Int32s stateNum;           /* number of states */

	AUD_Vector pi;                 /* initial state distribution */
	AUD_Matrix tMat;               /* transition matrix */
	void       **ppGmmModels;      /* observation GMMs */
	void       **ppAdaptGmmModels;
} GMM_HMM;

static void          ForwardWithScale( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pAlpha, AUD_Double *pScale, AUD_Double *pProb );
static void          BackwardWithScale( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pBeta, AUD_Double *pScale );
static void          ComputeGamma( GMM_HMM *pGmmhmm, AUD_Matrix *pAlpha, AUD_Matrix *pBeta, AUD_Matrix *pGamma );
static void          ComputeXi( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pAlpha, AUD_Matrix *pBeta, AUD_Double ***xi );
static void          MapAdapt( GMM_HMM *pGmmhmm, AUD_Feature *pFeatures, AUD_Matrix *pGamma, AUD_Int32s instanceNum, AUD_Double tau );

static AUD_Double*** AllocXi( AUD_Int32s T, AUD_Int32s N );
static void          FreeXi( AUD_Double ***xi, AUD_Int32s T, AUD_Int32s N );

AUD_Error gmmhmm_import( void **phGmmHmm, AUD_Int8s *pFileName )
{
	AUD_ASSERT( pFileName && phGmmHmm );

	AUDLOG( "import gmm-hmm model from: [%s]\n", pFileName );

	AUD_Error error;

	GMM_HMM *pState = (GMM_HMM*)calloc( 1, sizeof(GMM_HMM) );
	if ( pState == NULL )
	{
		*phGmmHmm = NULL;
		return AUD_ERROR_OUTOFMEMORY;
	}

	FILE *fp = fopen( (char*)pFileName, "rb" );
	if ( fp == NULL )
	{
		AUDLOG( "cannot open gmm-hmm model file: [%s]\n", pFileName );
		return AUD_ERROR_IOFAILED;
	}

	AUD_Int32s data, ret;

	// import state num
	data = fread( &(pState->stateNum), sizeof(AUD_Int32s), 1, fp );

	// import pi
	data = fread( &( pState->pi.dataType ), sizeof(pState->pi.dataType), 1, fp );
	data = fread( &( pState->pi.len ), sizeof(pState->pi.len), 1, fp );
	ret = createVector( &( pState->pi ) );
	AUD_ASSERT( ret == 0 );
	data = fread( (void*)( pState->pi.pInt16s ), SIZEOF_TYPE( pState->pi.dataType ) * pState->pi.len, 1, fp );

	// import transition matrix
	data = fread( &( pState->tMat.dataType ), sizeof(pState->tMat.dataType), 1, fp );
	data = fread( &( pState->tMat.rows ), sizeof(pState->tMat.rows), 1, fp );
	data = fread( &( pState->tMat.cols ), sizeof(pState->tMat.cols), 1, fp );
	ret = createMatrix( &( pState->tMat ) );
	AUD_ASSERT( ret == 0 );
	data = fread( (void*)( pState->tMat.pInt16s ), SIZEOF_TYPE( pState->tMat.dataType ) * pState->tMat.rows * pState->tMat.cols, 1, fp );

	// import Adapt GMM
	pState->ppAdaptGmmModels = (void**)calloc( pState->stateNum * sizeof(void*), 1 );
	AUD_ASSERT( pState->ppAdaptGmmModels );
	for ( int i = 0; i < pState->stateNum; i++ )
	{
		error = gmm_import( &(pState->ppAdaptGmmModels[i]), fp );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}

	fclose( fp );
	fp = NULL;

	*phGmmHmm = pState;

	return AUD_ERROR_NONE;
}

AUD_Error gmmhmm_export( void *hGmmHmm, AUD_Int8s *pFileName )
{
	AUD_ASSERT( pFileName && hGmmHmm );

	AUDLOG( "\n\nexport gmm-hmm model to: [%s]\n", pFileName );

	AUD_Error error;

	FILE *fp = fopen( (char*)pFileName, "wb" );
	if ( fp == NULL )
	{
		AUDLOG( "cannot open gmm-hmm model file: [%s]\n\n", pFileName );
		return AUD_ERROR_IOFAILED;
	}

	GMM_HMM *pState = (GMM_HMM*)hGmmHmm;

	// export state num
	(void)fwrite( &(pState->stateNum), sizeof(AUD_Int32s), 1, fp );

	// export pi
	(void)fwrite( &( pState->pi.dataType ), sizeof(pState->pi.dataType), 1, fp );
	(void)fwrite( &( pState->pi.len ), sizeof(pState->pi.len), 1, fp );
	(void)fwrite( (const void*)( pState->pi.pInt16s ), SIZEOF_TYPE( pState->pi.dataType ) * pState->pi.len, 1, fp );

	// export transition matrix
	(void)fwrite( &( pState->tMat.dataType ), sizeof(pState->tMat.dataType), 1, fp );
	(void)fwrite( &( pState->tMat.rows ), sizeof(pState->tMat.rows), 1, fp );
	(void)fwrite( &( pState->tMat.cols ), sizeof(pState->tMat.cols), 1, fp );
	(void)fwrite( (const void*)( pState->tMat.pInt16s ), SIZEOF_TYPE( pState->tMat.dataType ) * pState->tMat.rows * pState->tMat.cols, 1, fp );

	// export Adapt GMM
	for ( int i = 0; i < pState->stateNum; i++ )
	{
		error = gmm_export( pState->ppAdaptGmmModels[i], fp );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}

	fclose( fp );
	fp = NULL;

	return AUD_ERROR_NONE;
}

AUD_Error gmmhmm_show( void *hGmmHmm )
{
	AUD_ASSERT( hGmmHmm );

	AUDLOG( "gmm-hmm model detail:\n" );

	AUD_Error  error   = AUD_ERROR_NONE;;
	GMM_HMM    *pState = (GMM_HMM*)hGmmHmm;
	AUD_Int32s i, j;

	// state num
	AUDLOG( "state number: %d\n\n\n", pState->stateNum );

	// pi
	AUDLOG( "pi:\n" );
	AUDLOG( "\tlen: %d\n", pState->pi.len );
	for ( i = 0; i < pState->pi.len; i++ )
	{
		AUDLOG( "%f, ", pState->pi.pDouble[i] );
	}
	AUDLOG( "\n\n\n" );

	// transition matrix
	AUDLOG( "transition matrix:\n" );
	AUDLOG( "\trows: %d, cols: %d\n", pState->tMat.rows, pState->tMat.cols );
	for ( i = 0; i < pState->tMat.rows; i++ )
	{
		for ( j = 0; j < pState->tMat.cols; j++ )
		{
			AUDLOG( "%.3f, ", *( pState->tMat.pDouble + i * pState->tMat.cols + j ) );
		}
		AUDLOG( "\n\n" );
	}
	AUDLOG( "\n\n\n" );

	// export Adapt GMM
	for ( i = 0; i < pState->stateNum; i++ )
	{
		AUDLOG( "state %d GMM:\n", i );
		gmm_show( pState->ppAdaptGmmModels[i] );
		AUDLOG( "\n\n\n" );
	}

	return error;
}


AUD_Error gmmhmm_init( void **phGmmHmm, AUD_Int32s stateNum, AUD_Vector *pPi, void **ppSrcGmmModels )
{
	AUD_Error  error = AUD_ERROR_NONE;
	AUD_Int32s ret = 0;
	GMM_HMM    *pState = NULL;
	AUD_Int32s i, j;

	pState = (GMM_HMM*)calloc( 1, sizeof(GMM_HMM) );
	if ( pState == NULL )
	{
		*phGmmHmm = NULL;
		return AUD_ERROR_OUTOFMEMORY;
	}

	// init state num
	pState->stateNum = stateNum;

	// init transition matrix
	pState->tMat.rows     = pState->stateNum;
	pState->tMat.cols     = pState->stateNum;
	pState->tMat.dataType = AUD_DATATYPE_DOUBLE;
	ret = createMatrix( &(pState->tMat) );
	AUD_ASSERT( ret == 0 );

#if 0 // Ergodic Model
	for ( i = 0; i < pState->tMat.rows; i++ )
	{
		AUD_Double sum   = 0.0;
		AUD_Double *pRow = pState->tMat.pDouble + i * pState->tMat.cols;

		random_setseed( random_getseed() );
		for ( j = 0; j < pState->tMat.cols; j++ )
		{
			pRow[j] = random_getrand();
			sum += pRow[j];
		}
		for ( j = 0; j < pState->tMat.cols; j++ )
		{
			pRow[j] /= sum;
		}
	}
#else // Bakis / Left-Right Model
	for ( i = 0; i < pState->tMat.rows; i++ )
	{
		AUD_Double sum   = 0.0;
		AUD_Double *pRow = pState->tMat.pDouble + i * pState->tMat.cols;

		random_setseed( random_getseed() );
		// for ( j = i; j < pState->tMat.cols; j++ )
		for ( j = i; j < AUD_MIN( i + 2, pState->tMat.cols ); j++ )
		{
			pRow[j] = random_getrand();
			sum += pRow[j];
		}
		for ( j = i; j < pState->tMat.cols; j++ )
		{
			pRow[j] /= sum;
		}
	}
#endif

	// init observation GMMs
	pState->ppGmmModels = (void**)calloc( pState->stateNum * sizeof(void*), 1 );
	AUD_ASSERT( pState->ppGmmModels );
	for ( i = 0; i < pState->stateNum; i++ )
	{
		error = gmm_clone( (void**)&(pState->ppGmmModels[i]), ppSrcGmmModels[i], NULL );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}

	// init pi
	pState->pi.len      = pState->stateNum;
	pState->pi.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &(pState->pi) );
	AUD_ASSERT( ret == 0 );
	ret = cloneVector( &(pState->pi),  pPi );
	AUD_ASSERT( ret == 0 );

	*phGmmHmm = pState;

	return error;
}

AUD_Error gmmhmm_free( void **phGmmHmm )
{
	AUD_ASSERT( phGmmHmm != NULL && *phGmmHmm != NULL );

	GMM_HMM    *pState = (GMM_HMM*)( *phGmmHmm );
	AUD_Int32s i;
	AUD_Int32s ret = 0;

	// de-init state transition matrix
	ret = destroyMatrix( &(pState->tMat) );
	AUD_ASSERT( ret == 0 );

	// de-init observation GMMs
	if ( pState->ppGmmModels )
	{
		for ( i = 0; i < pState->stateNum; i++ )
		{
			gmm_free( (void**)&(pState->ppGmmModels[i]) );
		}
		free( pState->ppGmmModels );
	}

	// de-init adapt observation GMMS
	if ( pState->ppAdaptGmmModels )
	{
		for ( i = 0; i < pState->stateNum; i++ )
		{
			gmm_free( (void**)&(pState->ppAdaptGmmModels[i]) );
		}
		free( pState->ppAdaptGmmModels );
	}

	// de-init init distribution
	destroyVector( &(pState->pi) );

	free( pState );

	*phGmmHmm = NULL;

	return AUD_ERROR_NONE;
}

/* estimate transition matrix */
AUD_Error gmmhmm_learn( void *hGmmHmm, AUD_Feature *pFeatures, AUD_Int32s instanceNum, AUD_Double threshold )
{
	AUD_ASSERT( hGmmHmm && pFeatures && (instanceNum >= 1) );

	AUD_Int32s i = 0, j = 0, t = 0, m = 0;
	AUD_Int32s ret;

	GMM_HMM    *pState = (GMM_HMM*)hGmmHmm;
	AUD_Double delta;

	AUD_Matrix *pAlpha, *pBeta, *pGamma;
	AUD_Double **pScale;
	AUD_Double *pLogprob, *pLogprobprev;
	AUD_Double ****pXi;

	AUD_Matrix **pSamples;

	pSamples = (AUD_Matrix**)calloc( instanceNum, sizeof(AUD_Matrix*) );
	AUD_ASSERT( pSamples );

	pAlpha = (AUD_Matrix*)calloc( instanceNum, sizeof(AUD_Matrix) );
	AUD_ASSERT( pAlpha );

	pBeta = (AUD_Matrix*)calloc( instanceNum, sizeof(AUD_Matrix) );
	AUD_ASSERT( pBeta );

	pGamma = (AUD_Matrix*)calloc( instanceNum, sizeof(AUD_Matrix) );
	AUD_ASSERT( pGamma );

	pScale = (AUD_Double**)calloc( instanceNum, sizeof(AUD_Double*) );
	AUD_ASSERT( pScale );

	pLogprob = (AUD_Double*)calloc( instanceNum, sizeof(AUD_Double) );
	AUD_ASSERT( pLogprob );

	pLogprobprev = (AUD_Double*)calloc( instanceNum, sizeof(AUD_Double) );
	AUD_ASSERT( pLogprobprev );

	pXi = (AUD_Double****)calloc( instanceNum, sizeof(AUD_Double***) );
	AUD_ASSERT( pXi );

	for ( m = 0; m < instanceNum; m++ )
	{
		pSamples[m] = &(pFeatures[m].featureMatrix);

		// init alpha, beta, gamma
		pAlpha[m].rows     = pSamples[m]->rows;
		pAlpha[m].cols     = pState->stateNum;
		pAlpha[m].dataType = AUD_DATATYPE_DOUBLE;
		ret = createMatrix( &(pAlpha[m]) );
		AUD_ASSERT( ret == 0 );

		pBeta[m].rows     = pSamples[m]->rows;
		pBeta[m].cols     = pState->stateNum;
		pBeta[m].dataType = AUD_DATATYPE_DOUBLE;
		ret = createMatrix( &(pBeta[m]) );
		AUD_ASSERT( ret == 0 );

		pGamma[m].rows     = pSamples[m]->rows;
		pGamma[m].cols     = pState->stateNum;
		pGamma[m].dataType = AUD_DATATYPE_DOUBLE;
		ret = createMatrix( &(pGamma[m]) );
		AUD_ASSERT( ret == 0 );

		// init xi & scale vector
		pXi[m]     = AllocXi( pSamples[m]->rows, pState->stateNum );
		pScale[m]  = dvector( 0, pSamples[m]->rows - 1 );
	}

	for ( m = 0; m < instanceNum; m++ )
	{
		ForwardWithScale( pState, pSamples[m], &(pAlpha[m]), pScale[m], &pLogprob[m] );
		BackwardWithScale( pState, pSamples[m], &(pBeta[m]), pScale[m] );
		ComputeGamma( pState, &(pAlpha[m]), &(pBeta[m]), &(pGamma[m]) );
		ComputeXi( pState, pSamples[m], &(pAlpha[m]), &(pBeta[m]), pXi[m] );
		pLogprobprev[m] = pLogprob[m];
	}

	AUD_Double numeratorA, denominatorA;
	AUD_Int32s iter = 0;
	do
	{
		// re-estimate transition matrix
#if 0
		for ( i = 0; i < pState->stateNum; i++ )
		{
			denominatorA = 0.0;
			for ( t = 0; t < pSamples->rows - 1; t++ )
			{
				denominatorA += *( pState->gamma.pDouble + t * pState->gamma.cols + i );
			}

			for ( j = 0; j < pState->stateNum; j++ )
			{
				numeratorA = 0.0;
				for ( t = 0; t < pSamples->rows - 1; t++ )
				{
					numeratorA += xi[t][i][j];
				}
				*( pState->tMat.pDouble + i * pState->tMat.cols + j ) = numeratorA / denominatorA;
			}
		}
#else
		for ( i = 0; i < pState->stateNum; i++ )
		{
			denominatorA = LOG_ZERO;

			for ( m = 0; m < instanceNum; m++ )
			{
				for ( t = 0; t < pSamples[m]->rows - 1; t++ )
				{
					denominatorA = logadd( denominatorA, *( pGamma[m].pDouble + t * pGamma[m].cols + i ) );
				}
			}

			for ( j = 0; j < pState->stateNum; j++ )
			{
				if ( i <= j )
				{
					numeratorA = LOG_ZERO;
					for ( m = 0; m < instanceNum; m++ )
					{
						for ( t = 0; t < pSamples[m]->rows - 1; t++ )
						{
							numeratorA = logadd( numeratorA, pXi[m][t][i][j] );
						}
					}

					if ( numeratorA - denominatorA > 100. )
					{
						*( pState->tMat.pDouble + i * pState->tMat.cols + j ) = 0.;
					}
					else
					{
						*( pState->tMat.pDouble + i * pState->tMat.cols + j ) = ( numeratorA < denominatorA ) ? exp( numeratorA - denominatorA ) : 1.;
					}

					// AUDLOG( "numeratorA = %.3f, denominatorA = %.3f, A = %.3f\n", numeratorA, denominatorA, *( pState->tMat.pDouble + i * pState->tMat.cols + j ) );
				}
				else
				{
						*( pState->tMat.pDouble + i * pState->tMat.cols + j ) = 0.;
				}
			}
		}
#endif

		for ( m = 0; m < instanceNum; m++ )
		{
			ForwardWithScale( pState, pSamples[m], &(pAlpha[m]), pScale[m], &pLogprob[m] );
			BackwardWithScale( pState, pSamples[m], &(pBeta[m]), pScale[m] );
			ComputeGamma( pState, &(pAlpha[m]), &(pBeta[m]), &(pGamma[m]) );
			ComputeXi( pState, pSamples[m], &(pAlpha[m]), &(pBeta[m]), pXi[m] );
			pLogprobprev[m] = pLogprob[m];
		}

		/* compute difference between log probability of two iterations */
		delta = pLogprob[0] - pLogprobprev[0];
		for ( m = 1; m < instanceNum; m++ )
		{
			if ( delta < ( pLogprob[m] - pLogprobprev[m] ) )
			{
				delta = pLogprob[m] - pLogprobprev[m];
			}
		}

		for ( m = 0; m < instanceNum; m++ )
		{
			pLogprobprev[m] = pLogprob[m];
		}

		iter++;
	} while ( delta > threshold );

	for ( m = 0; m < instanceNum; m++ )
	{
		// destroy alpha, beta
		ret = destroyMatrix( &(pAlpha[m]) );
		AUD_ASSERT( ret == 0 );

		ret = destroyMatrix( &(pBeta[m]) );
		AUD_ASSERT( ret == 0 );

		FreeXi( pXi[m], pSamples[m]->rows, pState->stateNum );
		free_dvector( pScale[m], 0, pSamples[m]->rows - 1 );
	}

	free( pAlpha );
	pAlpha = NULL;
	free( pBeta );
	pBeta = NULL;
	free( pScale );
	pScale = NULL;
	free( pLogprob );
	pLogprob = NULL;
	free( pLogprobprev );
	pLogprobprev = NULL;
	free( pXi );
	pXi = NULL;
	free( pSamples );
	pSamples = NULL;

	AUDLOG( "*********************************\n" );
	AUDLOG( "\ttotal HMM learning iter num: %d\n", iter );
	AUDLOG( "*********************************\n" );

	// adaptation
	MapAdapt( pState, pFeatures, pGamma, instanceNum, WOV_HMM_MAP_TAU );

	for ( m = 0; m < instanceNum; m++ )
	{
		ret = destroyMatrix( &(pGamma[m]) );
		AUD_ASSERT( ret == 0 );
	}
	free( pGamma );
	pGamma = NULL;

	return AUD_ERROR_NONE;
}


AUD_Error gmmhmm_forward( void *hGmmHmm, AUD_Matrix *pSamples, AUD_Double *pProb )
{
	AUD_ASSERT( hGmmHmm && pSamples && pProb );

	AUD_Int32s i, j, t;
	AUD_Int32s ret = 0;

	AUD_Double sum;
	AUD_Double *pAlphaRow       = NULL;
	AUD_Double *pAlphaRowBefore = NULL;

	GMM_HMM *pState = (GMM_HMM*)hGmmHmm;

	AUD_Matrix alpha;
	alpha.rows     = pSamples->rows;
	alpha.cols     = pState->stateNum;
	alpha.dataType = AUD_DATATYPE_DOUBLE;
	ret = createMatrix( &alpha );
	AUD_ASSERT( ret == 0 );

	// AUDLOG( "pSamples->rows = %d, pState->stateNum = %d\n", pSamples->rows, pState->stateNum );
#if 0
	/* 1. Initialization */
	pAlphaRow = alpha.pDouble;
	for ( i = 0; i < alpha.cols; i++ )
	{
		pAlphaRow[i] = pState->pi.pDouble[i] * gmm_prob( pState->ppAdaptGmmModels[i], pSamples, 0, NULL );
		// AUDLOG( "time 0, state %d: pi: %f, pAlphaRow: %f, observatiom prob: %f, ", i, pState->pi.pDouble[i], pAlphaRow[i], gmm_prob( pState->ppAdaptGmmModels[i], pSamples, 0, NULL ) );
	}
	// AUDLOG( "\n\n" );

	/* 2. Induction */
	for ( t = 1; t < pSamples->rows; t++ )
	{
		pAlphaRow       = alpha.pDouble + t * alpha.cols;
		pAlphaRowBefore = alpha.pDouble + ( t - 1 ) * alpha.cols;

		for ( j = 0; j < pState->stateNum; j++ )
		{
			sum = 0.0f;
			for ( i = 0; i <= j; i++ )
			{
				sum += pAlphaRowBefore[i] * ( *(pState->tMat.pDouble + i * pState->tMat.cols + j) );
			}

			pAlphaRow[j] = sum * gmm_prob( pState->ppAdaptGmmModels[j], pSamples, t, NULL );

			// AUDLOG( "sum: %f, pAlphaRow[%d]: %f,  ", sum, j, pAlphaRow[j] );
		}
		// AUDLOG( "\n\n" );
	}

	// AUDLOG( "Evaluation alpha matrix(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
	// showMatrix( &alpha );

	/* 3. Termination */
	*pProb = 0.0;
	pAlphaRow = alpha.pDouble + ( pSamples->rows - 1 ) * alpha.cols;
	for ( i = 0; i < pState->stateNum; i++ )
	{
		*pProb += pAlphaRow[i];
	}

#else

	/* 1. Initialization */
	pAlphaRow = alpha.pDouble;
	pAlphaRow[0] = gmm_llr( pState->ppAdaptGmmModels[0], pSamples, 0, NULL );
	for ( i = 1; i < pState->stateNum; i++ )
	{
		pAlphaRow[i] = -10000. + gmm_llr( pState->ppAdaptGmmModels[i], pSamples, 0, NULL );
	}

	/* 2. Induction */
	for ( t = 1; t < pSamples->rows; t++ )
	{
		pAlphaRow       = alpha.pDouble + t * alpha.cols;
		pAlphaRowBefore = alpha.pDouble + ( t - 1 ) * alpha.cols;

		for ( j = 0; j < pState->stateNum; j++ )
		{
			sum = 0.;
			for ( i = 0; i < pState->stateNum; i++ )
			{
				if ( *(pState->tMat.pDouble + i * pState->tMat.cols + j) > 0. )
				{
					if ( sum == 0. )
					{
						sum = pAlphaRowBefore[i] + log( *(pState->tMat.pDouble + i * pState->tMat.cols + j) );
					}
					else
					{
						sum = logadd( sum, pAlphaRowBefore[i] + log( *(pState->tMat.pDouble + i * pState->tMat.cols + j) ) );
					}
				}
			}

			pAlphaRow[j] = sum + gmm_llr( pState->ppAdaptGmmModels[j], pSamples, t, NULL );
		}
	}

	/* 3. Termination */
	pAlphaRow = alpha.pDouble + ( pSamples->rows - 1 ) * alpha.cols;
	*pProb = pAlphaRow[0];
	for ( i = 1; i < pState->stateNum; i++ )
	{
		*pProb = logadd( *pProb, pAlphaRow[i] );
	}

#endif

	ret = destroyMatrix( &alpha );
	AUD_ASSERT( ret == 0 );

	return AUD_ERROR_NONE;
}

static void MapAdapt( GMM_HMM *pGmmhmm, AUD_Feature *pFeatures, AUD_Matrix *pGamma, AUD_Int32s instanceNum, AUD_Double tau )
{
	AUD_Int32s  t, i, j, k, m, ret;
	AUD_Double  c, acc;

	AUD_Matrix  **pSamples;

	pSamples = (AUD_Matrix**)calloc( instanceNum, sizeof(AUD_Matrix*) );
	AUD_ASSERT( pSamples )

	// init adapt GMM models
	if ( pGmmhmm->ppAdaptGmmModels == NULL )
	{
		pGmmhmm->ppAdaptGmmModels = (void**)calloc( pGmmhmm->stateNum * sizeof(void*), 1 );
		AUD_ASSERT( pGmmhmm->ppAdaptGmmModels );

		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			gmm_clone( (void**)(&(pGmmhmm->ppAdaptGmmModels[i])), (void*)(pGmmhmm->ppGmmModels[i]), NULL );
		}
	}

	for ( m = 0; m < instanceNum; m++ )
	{
		pSamples[m] = &(pFeatures[m].featureMatrix);
	}

	AUD_Double **pC = (AUD_Double**)malloc( sizeof(AUD_Double*) * instanceNum );
	AUD_ASSERT( pC );
	for ( m = 0; m < instanceNum; m++ )
	{
		pC[m] = (AUD_Double*)malloc( sizeof(AUD_Double) * pSamples[m]->rows );
		AUD_ASSERT( pC[m] );
	}


#if 0

	AUD_Double prob;
	AUD_Vector compProb;
	compProb.len      = gmm_getmixnum( pGmmhmm->ppGmmModels[0] );
	compProb.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &compProb );
	AUD_ASSERT( ret == 0 );

	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		GmmModel   *pGMM      = (GmmModel*)pGmmhmm->ppGmmModels[i];
		GmmModel   *pGMMAdapt = (GmmModel*)pGmmhmm->ppAdaptGmmModels[i];
		AUD_Matrix *pMapMeans = &(pGMMAdapt->means);
		for ( j = 0; j < pGMM->numMix; j++ )
		{
			c   = 0.;
			for ( t = 0; t < pSamples->rows; t++ )
			{
				prob = gmm_prob( pGMM, pSamples, t, &compProb );
				pC[t] = *(pGmmhmm->gamma.pDouble + i * pSamples->rows + t) * compProb.pDouble[j] / prob;
				c += pC[t];
			}

			for ( k = 0; k < pGMM->width; k++ )
			{
				acc = 0.;
				for ( t = 0; t < pSamples->rows; t++ )
				{
					acc +=  pC[t] * ( *(pSamples->pInt32s + t * pSamples->cols + k) );
				}

				*(pMapMeans->pInt32s + j * pGMM->width + k) = ( tau * ( *(pGMM->means.pInt32s + j * pGMM->step + k) ) + acc ) /
                                                              ( tau + c );
			}
		}
	}

	destroyVector( &compProb );

#else

	AUD_Double llr;
	AUD_Vector compLLR;
	compLLR.len      = gmm_getmixnum( pGmmhmm->ppGmmModels[0] );
	compLLR.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &compLLR );
	AUD_ASSERT( ret == 0 );

	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		GmmModel   *pGMM      = (GmmModel*)pGmmhmm->ppGmmModels[i];
		GmmModel   *pGMMAdapt = (GmmModel*)pGmmhmm->ppAdaptGmmModels[i];
		AUD_Matrix *pMapMeans = &(pGMMAdapt->means);
		for ( j = 0; j < pGMM->numMix; j++ )
		{
			c = LOG_ZERO;
			// AUDLOG( "gamma, comp llr, llr, pC:\n" );
			for ( m = 0; m < instanceNum; m++ )
			{
				for ( t = 0; t < pSamples[m]->rows; t++ )
				{
					llr = gmm_llr( pGMM, pSamples[m], t, &compLLR );
					pC[m][t] = *(pGamma[m].pDouble + i * pSamples[m]->rows + t) + compLLR.pDouble[j] - llr;

					c = logadd( c, pC[m][t] );
					// AUDLOG( "%.3f, %.3f, %.3f, %.3f\n", *(pGmmhmm->gamma.pDouble + i * pSamples->rows + t), compLLR.pDouble[j], llr, pC[t] );
				}
			}
			// AUDLOG( "\n" );

			for ( k = 0; k < pGMM->width; k++ )
			{
				acc = 0.;
				for ( m = 0; m < instanceNum; m++ )
				{
					for ( t = 0; t < pSamples[m]->rows; t++ )
					{
						acc += exp( pC[m][t] ) * ( *(pSamples[m]->pInt32s + t * pSamples[m]->cols + k) );
					}
				}

				// AUDLOG( "c: %f, acc: %f\n", c, acc );
				*(pMapMeans->pInt32s + j * pGMM->width + k) = ( tau * ( *(pGMM->means.pInt32s + j * pGMM->step + k) ) + acc ) /
                                                              ( tau + exp( c ) );
			}
		}
	}

	destroyVector( &compLLR );

#endif

	for ( m = 0; m < instanceNum; m++ )
	{
		free( pC[m] );
	}
	free( pC );
	pC = NULL;
	free( pSamples );
	pSamples = NULL;

	return;
}


/*  pProb is the LOG probability */
static void ForwardWithScale( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pAlpha, AUD_Double *pScale, AUD_Double *pProb )
{
	AUD_Int32s i, j, t;
	AUD_Double sum;
	AUD_Double *pAlphaRow       = NULL;
	AUD_Double *pAlphaRowBefore = NULL;

#if 0

	/* 1. Initialization */
	pScale[0] = 0.0;
	pAlphaRow = pAlpha->pDouble;
	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		pAlphaRow[i] = pGmmhmm->pi.pDouble[i] * gmm_prob( pGmmhmm->ppGmmModels[i], pSamples, 0, NULL );
		pScale[0] += pAlphaRow[i];
	}
	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		pAlphaRow[i] /= pScale[0];
	}

	/* 2. Induction */
	for ( t = 1; t < pSamples->rows; t++ )
	{
		pScale[t] = 0.0;
		pAlphaRow = pAlpha->pDouble + t * pAlpha->cols;
		pAlphaRowBefore = pAlpha->pDouble + ( t - 1 ) * pAlpha->cols;

		for ( j = 0; j < pGmmhmm->stateNum; j++ )
		{
			sum = 0.0;
			for ( i = 0; i < pGmmhmm->stateNum; i++ )
			{
				sum += pAlphaRowBefore[i] * ( *(pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols + j) );
			}

			pAlphaRow[j] = sum * gmm_prob( pGmmhmm->ppGmmModels[j], pSamples, t, NULL );
			pScale[t] += pAlphaRow[j];
		}

		for ( j = 0; j < pGmmhmm->stateNum; j++ )
		{
			pAlphaRow[j] /= pScale[t];
		}
	}

	/* 3. Termination */
	*pProb = 0.0;
	for ( t = 0; t < pSamples->rows; t++ )
	{
		*pProb += pScale[t];
	}

#else

	/* 1. Initialization */
	pAlphaRow = pAlpha->pDouble;
	pAlphaRow[0] = gmm_llr( pGmmhmm->ppGmmModels[0], pSamples, 0, NULL );
	for ( i = 1; i < pGmmhmm->stateNum; i++ )
	{
		pAlphaRow[i] = -10000. + gmm_llr( pGmmhmm->ppGmmModels[i], pSamples, 0, NULL );
	}

	/* 2. Induction */
	for ( t = 1; t < pSamples->rows; t++ )
	{
		pAlphaRow       = pAlpha->pDouble + t * pAlpha->cols;
		pAlphaRowBefore = pAlpha->pDouble + ( t - 1 ) * pAlpha->cols;

		for ( j = 0; j < pGmmhmm->stateNum; j++ )
		{
			sum = 0.;
			for ( i = 0; i < pGmmhmm->stateNum; i++ )
			{
				if ( *(pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols + j) > 0. )
				{
					if ( sum == 0. )
					{
						sum = pAlphaRowBefore[i] + log( *(pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols + j) );
					}
					else
					{
						sum = logadd( sum, pAlphaRowBefore[i] + log( *(pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols + j) ) );
					}
				}
			}

			pAlphaRow[j] = sum + gmm_llr( pGmmhmm->ppGmmModels[j], pSamples, t, NULL );
		}
	}

	/* 3. Termination */
	pAlphaRow = pAlpha->pDouble + ( pSamples->rows - 1 ) * pAlpha->cols;
	*pProb = pAlphaRow[0];
	for ( i = 1; i < pGmmhmm->stateNum; i++ )
	{
		*pProb = logadd( *pProb, pAlphaRow[i] );
	}

#endif

#if 0
	AUDLOG( "************************ALPHA\n" );
	for ( t = 0; t < pSamples->rows; t++ )
	{
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			AUDLOG( "%f, ", *(pAlpha->pDouble + t * pAlpha->cols + i) );
		}
		AUDLOG( "alpha line %d \n\n", t );
	}
	AUDLOG( "\n\n\n" );
#endif

	return;
}

static void BackwardWithScale( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pBeta, AUD_Double *pScale )
{
	AUD_Int32s i, j, t;
	AUD_Double sum;
	AUD_Double *pBetaRow      = NULL;
	AUD_Double *pBetaRowAfter = NULL;

#if 0

	/* 1. Initialization */
	pBetaRow = pBeta->pDouble + (pSamples->rows - 1) * pBeta->cols;
	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		pBetaRow[i] = 1.0 / pScale[pSamples->rows - 1];
	}

	/* 2. Induction */
	AUD_Double *pTMatRow = NULL;
	for ( t = pSamples->rows - 2; t >= 0; t-- )
	{
		pBetaRow      = pBeta->pDouble + t * pBeta->cols;
		pBetaRowAfter = pBeta->pDouble + ( t + 1 ) * pBeta->cols;
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			sum      = 0.0;
			pTMatRow = pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols;

			for ( j = 0; j < pGmmhmm->stateNum; j++ )
			{
				sum += pTMatRow[j] * gmm_prob( pGmmhmm->ppGmmModels[j], pSamples, t + 1, NULL ) * pBetaRowAfter[j];
			}

			pBetaRow[i] = sum / pScale[t];
		}
	}

#else

	/* 1. Initialization */
	pBetaRow = pBeta->pDouble + (pSamples->rows - 1) * pBeta->cols;
	for ( i = 0; i < pGmmhmm->stateNum; i++ )
	{
		pBetaRow[i] = 0.;
	}

	/* 2. Induction */
	AUD_Double *pTMatRow = NULL;
	AUD_Double item      = 0.;
	for ( t = pSamples->rows - 2; t >= 0; t-- )
	{
		pBetaRow      = pBeta->pDouble + t * pBeta->cols;
		pBetaRowAfter = pBeta->pDouble + ( t + 1 ) * pBeta->cols;
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			pTMatRow = pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols;

			sum = log( pTMatRow[i] ) + gmm_llr( pGmmhmm->ppGmmModels[i], pSamples, t + 1, NULL ) + pBetaRowAfter[i];
			for ( j = i + 1; j < pGmmhmm->stateNum; j++ )
			{
				if ( pTMatRow[j] != 0 )
				{
					item = log( pTMatRow[j] ) + gmm_llr( pGmmhmm->ppGmmModels[j], pSamples, t + 1, NULL ) + pBetaRowAfter[j];
					sum = logadd( sum, item );
					// AUDLOG( "item: %f, sum: %f\n", sum );
				}
			}

			pBetaRow[i] = sum;
		}
	}

#endif

#if 0
	AUDLOG( "++++++++++++++++++++++++++BETA\n" );
	for ( t = 0; t < pSamples->rows; t++ )
	{
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			AUDLOG( "%f, ", *( pBeta->pDouble + t * pGmmhmm->stateNum + i ) );
		}
		AUDLOG( "beta line %d\n", t );
	}
	AUDLOG( "\n\n\n" );
	AUDLOG( "++++++++++++++++++++++++++BETA Finish\n\n\n" );
#endif

	return;
}

static void ComputeGamma( GMM_HMM *pGmmhmm, AUD_Matrix *pAlpha, AUD_Matrix *pBeta, AUD_Matrix *pGamma )
{
	AUD_Int32s i, t;
	AUD_Double normalizer;

#if 0

	for ( t = 0; t < pAlpha->rows; t++ )
	{
		AUD_Double *pAlphaRow = pAlpha->pDouble + t * pAlpha->cols;
		AUD_Double *pBetaRow  = pBeta->pDouble  + t * pBeta->cols;
		AUD_Double *pGammaRow = pGamma->pDouble + t * pGamma->cols;

		normalizer = 0.0;
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			pGammaRow[i] = pAlphaRow[i] * pBetaRow[i];
			normalizer += pGammaRow[i];
		}

		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			pGammaRow[i] = pGammaRow[i] / normalizer;
		}
	}

#else

	for ( t = 0; t < pAlpha->rows; t++ )
	{
		AUD_Double *pAlphaRow = pAlpha->pDouble + t * pAlpha->cols;
		AUD_Double *pBetaRow  = pBeta->pDouble  + t * pBeta->cols;
		AUD_Double *pGammaRow = pGamma->pDouble + t * pGamma->cols;

		pGammaRow[0] = pAlphaRow[0] + pBetaRow[0];
		normalizer   = pGammaRow[0];
		for ( i = 1; i < pGmmhmm->stateNum; i++ )
		{
			pGammaRow[i] = pAlphaRow[i] + pBetaRow[i];
			normalizer   = logadd( normalizer, pGammaRow[i] );
		}

		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			pGammaRow[i] = pGammaRow[i] - normalizer;
		}
	}

#endif

	return;
}

static void ComputeXi( GMM_HMM *pGmmhmm, AUD_Matrix *pSamples, AUD_Matrix *pAlpha, AUD_Matrix *pBeta, AUD_Double ***xi )
{
	AUD_Int32s i, j, t;
	AUD_Double sum;

#if 0

	for ( t = 0; t < pSamples->rows - 1; t++ )
	{
		sum = 0.0;
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			AUD_Double *pAlphaRow = pAlpha->pDouble + t * pAlpha->cols;
			AUD_Double *pBetaRow  = pBeta->pDouble + (t + 1) * pBeta->cols;
			AUD_Double *pTMatRow  = pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols;

			for ( j = 0; j < pGmmhmm->stateNum; j++ )
			{
				xi[t][i][j] = pAlphaRow[i] * pTMatRow[j] * pBetaRow[j]
				              * gmm_prob( pGmmhmm->ppGmmModels[j], pSamples, t + 1, NULL );
				sum += xi[t][i][j];
			}
		}

		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			for ( j = 0; j < pGmmhmm->stateNum; j++ )
			{
				xi[t][i][j] /= sum;
			}
		}
	}

#else

	for ( t = 0; t < pSamples->rows - 1; t++ )
	{
		sum = 0.0;
		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			AUD_Double *pAlphaRow = pAlpha->pDouble + t * pAlpha->cols;
			AUD_Double *pBetaRow  = pBeta->pDouble + (t + 1) * pBeta->cols;
			AUD_Double *pTMatRow  = pGmmhmm->tMat.pDouble + i * pGmmhmm->tMat.cols;

			for ( j = i; j < pGmmhmm->stateNum; j++ )
			{
				if ( pTMatRow[j] > 0 )
				{
					xi[t][i][j] = pAlphaRow[i] + log( pTMatRow[j] ) + pBetaRow[j] +
					              gmm_llr( pGmmhmm->ppGmmModels[j], pSamples, t + 1, NULL );

					if ( sum == 0. )
					{
						sum = xi[t][i][j];
					}
					else
					{
						sum = logadd( sum, xi[t][i][j] );
					}
				}
			}
		}

		for ( i = 0; i < pGmmhmm->stateNum; i++ )
		{
			for ( j = 0; j < pGmmhmm->stateNum; j++ )
			{
				xi[t][i][j] -= sum;
			}
		}
	}

#endif

	return;
}


static AUD_Double*** AllocXi( AUD_Int32s T, AUD_Int32s N )
{
	AUD_Int32s    t;
	AUD_Double ***xi = NULL;

	xi = (AUD_Double***)malloc( T * sizeof(AUD_Double**) );
	if ( xi == NULL )
	{
		return NULL;
	}

	for ( t = 0; t < T; t++ )
	{
		xi[t] = dmatrix( 0, N - 1, 0, N - 1 );
	}

	return xi;
}

static void FreeXi( AUD_Double ***xi, AUD_Int32s T, AUD_Int32s N )
{
	AUD_Int32s t;

	for ( t = 0; t < T; t++ )
	{
		free_dmatrix( xi[t], 0, N - 1, 0, N - 1 );
	}

	free( xi );

	return;
}
