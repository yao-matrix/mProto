#include <cv.h>
#include <ml.h>
#include <cxcore.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

using namespace std;
using namespace cv;

AUD_Error kmeans_cluster( AUD_Matrix *pSamples, AUD_Int32s numCluster, AUD_Int32s maxInteration, AUD_Double minEps, AUD_Int32s attempts,
                          AUD_Matrix *pClusterLabel, AUD_Matrix *pClusterCentroid )
{
	AUD_ASSERT( pSamples != NULL );
	AUD_ASSERT( pClusterLabel != NULL );
	AUD_ASSERT( pClusterCentroid != NULL );
	AUD_ASSERT( numCluster > 1 );
	AUD_ASSERT( pClusterCentroid->dataType == AUD_DATATYPE_INT32S );

	AUD_Int32s cvDataType;

	switch ( pSamples->dataType )
	{
		case AUD_DATATYPE_INT16S:
			cvDataType = CV_16SC1;
			break;

		case AUD_DATATYPE_INT32S:
			cvDataType = CV_32SC1;
			break;

		case AUD_DATATYPE_DOUBLE:
			cvDataType = CV_64FC1;
			break;

		default:
			AUD_ASSERT( 0 );
			break;
	}

	// XXX: make assumption all types of pointer has same byte length here
	Mat cvInSampleMatrix = Mat( pSamples->rows, pSamples->cols, cvDataType, (void*)(pSamples->pInt32s) );
	Mat cvSampleMatrix   = Mat( pSamples->rows, pSamples->cols, CV_32FC1 );
	cvInSampleMatrix.convertTo( cvSampleMatrix, CV_32FC1, 1., 0 );

	Mat cvClusterLabel    = Mat( pClusterLabel->rows, pClusterLabel->cols, CV_32SC1, (void*)(pClusterLabel->pInt32s) );

	Mat cvClusterCentroid = Mat( pClusterCentroid->rows, pClusterCentroid->cols, CV_32FC1 );

	kmeans( cvSampleMatrix, numCluster, cvClusterLabel, cvTermCriteria( CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, maxInteration, minEps ), attempts, KMEANS_PP_CENTERS, cvClusterCentroid );

	AUD_Int32s i, j;
	for ( i = 0; i < pClusterCentroid->rows; i++ )
	{
		for ( j = 0; j < pClusterCentroid->cols; j++ )
		{
			*(pClusterCentroid->pInt32s + i * pClusterCentroid->cols + j) = (AUD_Int32s)round( cvClusterCentroid.at<float>( i, j ) );
		}
	}

	return AUD_ERROR_NONE;
}

