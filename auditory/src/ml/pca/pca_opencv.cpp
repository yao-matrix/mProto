#include <cv.h>
#include <ml.h>
#include <cxcore.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

using namespace std;
using namespace cv;

// [Q] How to choose number of principal components of PCA analysis?
// [A] use Scree plot(refer to:
// http://globin.cse.psu.edu/courses/spring2002/6_dim_red_II.pdf)


/*
 *  [IN]  pSamples     : sample matrix, which each sample as a row of
 *                       matrix;
 *        maxComponents: max number of principal components required
 *  [OUT] pAvg         : mean vector derived
 *        pEigenVec    : eigen vector derived
 *  [RET] error code
 * 
 */
AUD_Error pca_calc( AUD_Matrix *pSamples, AUD_Int32s maxComponents, AUD_Matrix *pAvg, AUD_Matrix *pEigenVec )
{
	AUD_ASSERT( pSamples != NULL );
	AUD_ASSERT( pAvg != NULL );
	AUD_ASSERT( pEigenVec != NULL );
	AUD_ASSERT( maxComponents >= 1 && maxComponents <= pSamples->cols );
	AUD_ASSERT( pSamples->rows > pSamples->cols );
	AUD_ASSERT( pSamples->dataType == AUD_DATATYPE_INT32S && pAvg->dataType == AUD_DATATYPE_INT32S && pEigenVec->dataType == AUD_DATATYPE_FLOAT );

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
	cvInSampleMatrix.convertTo( cvSampleMatrix, CV_32FC1 );

	CvMat *avg      = cvCreateMat( 1, pSamples->cols, CV_32FC1 );
	CvMat *eigenval = cvCreateMat( 1, maxComponents, CV_32FC1 );
	CvMat *eigenvec = cvCreateMat( maxComponents, pSamples->cols, CV_32FC1 );

	CvMat sample = cvSampleMatrix;

	cvCalcPCA( &sample, avg, eigenval, eigenvec, CV_PCA_DATA_AS_ROW );

	AUD_Int32s i, j;

	for ( i = 0; i < pSamples->cols; i++ )
	{
		*(pAvg->pInt32s + i) = (AUD_Int32s)round( CV_MAT_ELEM( *avg, float, 0, i ) );
	}

	for ( i = 0; i < maxComponents; i++ )
	{
		for ( j = 0; j < pSamples->cols; j++ )
		{
			*(pEigenVec->pFloat + i * pSamples->cols + j) = CV_MAT_ELEM( *eigenvec, float, i, j );
		}
	}

	cvReleaseMat( &avg );
	cvReleaseMat( &eigenval );
	cvReleaseMat( &eigenvec );

	return AUD_ERROR_NONE;
}
