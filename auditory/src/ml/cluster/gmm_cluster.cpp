/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

// system lib
#include <math.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"

#include "type/matrix.h"
#include "../gmm/gmm.h"

static AUD_Float bhat_dist( AUD_Int32s *pMean1, AUD_Int64s *pCov1, AUD_Int32s *pMean2, AUD_Int64s *pCov2, AUD_Int32s width )
{
	// Suppose a diagonal coveriance matrix
	// Db = 0.125 * (m1 - m2)' * Inv(P) * (m1 - m2) + 0.5 * ln(detP / sqrt( detP1 * detP2 ))
	// P = 0.5 * ( P1 + P2 )

	AUD_Int32s i           = 0;
	AUD_Int32s *pMeanDelta = (AUD_Int32s*)calloc( sizeof(AUD_Int32s) * width, 1 );
	AUD_ASSERT( pMeanDelta );
	for ( i = 0; i < width; i++ )
	{
		pMeanDelta[i] = pMean1[i] - pMean2[i];
		// AUDLOG( "pMeanDelta[%d]: %d, ", i, pMeanDelta[i] );
	}
	// AUDLOG( "\n\n" );

	AUD_Double *pCovAverage = (AUD_Double*)calloc( sizeof(AUD_Double) * width, 1 );
	AUD_ASSERT( pCovAverage );
	for ( i = 0; i < width; i++ )
	{
		pCovAverage[i] = ( pCov1[i] + pCov2[i] ) / 2.;
	}

	AUD_Double detCovRatio = 1.;
	for ( i = 0; i < width; i++ )
	{
		detCovRatio *= ( pCovAverage[i] / ( sqrt( (AUD_Double)pCov1[i] * (AUD_Double)pCov2[i] ) ) );
	}

	AUD_Double item1 = 0., item2 = 0.;
	for ( i = 0; i < width; i++ )
	{
		if ( pCovAverage[i] != 0. )
		{
			item1 += pow( (AUD_Double)pMeanDelta[i], 2. ) / pCovAverage[i];
		}
		else
		{
			AUDLOG( "!!! pCovAverage[%d] == 0\n", i );
			AUD_ASSERT( 0 );
		}
	}
	item1 = item1 / 8.;

	item2 = 0.5 * log( detCovRatio );

	// AUDLOG( "item1: %f, item2: %f\n", item1, item2 );
	free( pMeanDelta );
	pMeanDelta = NULL;

	free( pCovAverage );
	pCovAverage = NULL;

	return ( item1 + item2 );
}

AUD_Error gmm_cluster( void *hGmmModel, AUD_Matrix *pIndexTable, AUD_Double threshold, AUD_Int32s *pClusterLabel )
{
	AUD_ASSERT( hGmmModel && pIndexTable && pClusterLabel );
	AUD_ASSERT( pIndexTable->rows > 0 && pIndexTable->cols > 0 && pIndexTable->dataType == AUD_DATATYPE_INT32S );
	AUD_ASSERT( threshold > 0 );

	AUD_Int32s i          = 0;
	AUD_Int32s curLabel   = 0;
	AUD_Int32s labelPop   = 0;
	GmmModel   *pGmmState = (GmmModel*)hGmmModel;

	AUD_Int32s *pMean1   = NULL;
	AUD_Int32s *pMean2   = NULL;
	AUD_Int64s *pCov1    = NULL;
	AUD_Int64s *pCov2    = NULL;

	AUD_Float *pResult = (AUD_Float*)calloc( pIndexTable->rows * sizeof(AUD_Float), 1 );
	AUD_ASSERT( pResult );

	pMean1 = pGmmState->means.pInt32s + ( *(pIndexTable->pInt32s) ) * pGmmState->means.cols;
	pCov1  = pGmmState->cvars.pInt64s + ( *(pIndexTable->pInt32s) ) * pGmmState->cvars.cols;
	for ( i = 1; i < pIndexTable->rows; i++ )
	{
		AUD_Int32s index = *(pIndexTable->pInt32s + i * pIndexTable->cols);

		pMean2 = pGmmState->means.pInt32s + index * pGmmState->means.cols;
		pCov2  = pGmmState->cvars.pInt64s + index * pGmmState->cvars.cols;

		pResult[i] = bhat_dist( pMean1, pCov1, pMean2, pCov2, pGmmState->width );

		pMean1  = pMean2;
		pCov1   = pCov2;
	}

	// cluster
	curLabel         = 0;
	labelPop         = 0;
	pClusterLabel[0] = curLabel;
	for ( i = 1; i < pIndexTable->rows; i++ )
	{
		if ( pResult[i] < threshold )
		{
			pClusterLabel[i] = curLabel;
		}
		else
		{
			if ( labelPop > 1 )
			{
				curLabel++;
			}
			else // == 1
			{
				pClusterLabel[i - 1] = -1;
			}
			pClusterLabel[i] = curLabel;
			labelPop = 0;
		}
		labelPop++;
	}

#if 0
	for ( i = 1; i < pIndexTable->rows; i++ )
	{
		AUDLOG( "[%d: distance: %.2f, cluster label: %d]\n", i, pResult[i], pClusterLabel[i] );
	}

	if ( (pClusterLabel[pIndexTable->rows - 1] + 1) < 5 )
	{
		AUDLOG( "classified to %d states, fewer than expectation, pls try to lower your threshold and try again\n", pClusterLabel[pIndexTable->rows - 1] + 1 );
	}
#endif

	free( pResult );
	pResult = NULL;

	return AUD_ERROR_NONE;
}
