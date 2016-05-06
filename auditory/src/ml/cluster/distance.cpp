/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <math.h>
 
#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

static AUD_Double euclideanDist( AUD_Int32s* pVec1, AUD_Int32s* pVec2, AUD_Int32s len )
{
	AUD_Double dist = 0.;
	AUD_Int32s i;

	for ( i = 0; i < len; i++ )
	{
		dist += pow( pVec1[i] - pVec2[i], 2 );
	}
	dist = sqrt( dist );
	// AUDLOG( "dist: %f\n", dist );

	return dist;
}

// get min distance to cluster using Euclidean distance
AUD_Double calc_nearestdist( AUD_Matrix *pFeatMatrix, AUD_Int32s rowIndex, AUD_Matrix *pCluster )
{
	AUD_ASSERT( pFeatMatrix != NULL );
	AUD_ASSERT( pCluster != NULL );
	AUD_ASSERT( pFeatMatrix->dataType == AUD_DATATYPE_INT32S );
	AUD_ASSERT( pCluster->dataType == AUD_DATATYPE_INT32S );
	AUD_ASSERT( rowIndex >= 0 && rowIndex < pFeatMatrix->rows );
	AUD_ASSERT( pFeatMatrix->cols == pCluster->cols && pCluster->cols > 0 );

	AUD_Int32s i;
	AUD_Double minDistance = -1.;
	AUD_Double curDistance;

	AUD_Int32s *pVec = pFeatMatrix->pInt32s + rowIndex * pFeatMatrix->cols;

	for ( i = 0; i < pCluster->rows; i++ )
	{
		curDistance = euclideanDist( pVec, pCluster->pInt32s + i * pCluster->cols, pCluster->cols );
		if ( curDistance < minDistance || minDistance < 0 )
		{
			minDistance = curDistance;
		}
	}

	return minDistance;
}

