#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

#include "type/matrix.h"

/*
 *  [IN]  pSamples     : sample matrix, which each sample as a row of
 *                       matrix;
 *        pAvg         : mean vector;
 *        pTransMatrix : transform matrix, whose each coloum is a
 *                       principal component
 *  [OUT] pOutPca      : dimension reduced output
 *  [RET] error code
 * 
 */
AUD_Error pca_project( AUD_Matrix *pSamples, AUD_Matrix *pAvg, AUD_Matrix *pTransMatrix, AUD_Matrix *pOutPca )
{
	AUD_ASSERT( pSamples != NULL && pOutPca != NULL );
	AUD_ASSERT( pAvg != NULL );
	AUD_ASSERT( pTransMatrix != NULL );
	AUD_ASSERT( pSamples->cols == pTransMatrix->rows && pTransMatrix->cols == pOutPca->cols );
	AUD_ASSERT( pAvg->rows == 1 && pSamples->cols == pAvg->cols );
	AUD_ASSERT( pSamples->rows == pOutPca->rows );
	AUD_ASSERT( pSamples->dataType == AUD_DATATYPE_INT32S );

	// level shift
	AUD_Int32s i = 0, j = 0;
	for ( i = 0; i < pSamples->rows; i++ )
	{
		for ( j = 0; j < pSamples->cols; j++ )
		{
			*( pSamples->pInt32s + i * pSamples->cols + j ) -= GET_MATRIX_ELEMENT( pAvg, 1, j );
		}
	}

	// dimension reduction
	AUD_Int32s ret = 0;
	ret = multiplyMatrix( pSamples, pTransMatrix, pOutPca );
	AUD_ASSERT( ret == 0 );


	return AUD_ERROR_NONE;
}

