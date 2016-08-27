#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include "AudioUtil.h"

#include "matrix.h"
#include "math/mmath.h"


// matrix
AUD_Int32s createMatrix( AUD_Matrix *pMatrix )
{
	AUD_ASSERT( pMatrix );
	AUD_ASSERT( pMatrix->rows > 0 && pMatrix->cols > 0 );

	AUD_Int32s eleSize = SIZEOF_TYPE( pMatrix->dataType );
	AUD_ASSERT( eleSize != -1 );

	// since union member share the value, do just allocate one of them is OK
	pMatrix->pInt16s = (AUD_Int16s*)calloc( pMatrix->rows * pMatrix->cols * eleSize, 1 );
	if ( pMatrix->pInt16s == NULL )
	{
		return -1;
	}

	return 0;
}

AUD_Int32s cleanMatrix( AUD_Matrix *pMatrix )
{
	AUD_ASSERT( pMatrix );
	AUD_ASSERT( pMatrix->rows > 0 && pMatrix->cols > 0 );
	AUD_ASSERT( pMatrix->pInt16s );

	AUD_Int32s eleSize = SIZEOF_TYPE( pMatrix->dataType );
	AUD_ASSERT( eleSize != -1 );

	memset( pMatrix->pInt16s, 0, pMatrix->rows * pMatrix->cols * eleSize );

	return 0;
}

AUD_Int32s cloneMatrix( AUD_Matrix *pDstMatrix,  AUD_Matrix *pSrcMatrix )
{
	AUD_ASSERT( pDstMatrix && pSrcMatrix );
	AUD_ASSERT( pDstMatrix->rows == pSrcMatrix->rows && pDstMatrix->cols == pSrcMatrix->cols );
	AUD_ASSERT( pDstMatrix->dataType == pSrcMatrix->dataType );

	memmove( pDstMatrix->pInt16s, pSrcMatrix->pInt16s, pDstMatrix->rows * pDstMatrix->cols * SIZEOF_TYPE( pDstMatrix->dataType ) );

	return 0;
}

AUD_Int32s mergeMatrix( AUD_Matrix *pDstMatrix,  AUD_Matrix *pSrcMatrix1, AUD_Matrix *pSrcMatrix2 )
{
	AUD_ASSERT( pDstMatrix && pSrcMatrix1 && pSrcMatrix2 );
	AUD_ASSERT( pDstMatrix->rows == (pSrcMatrix1->rows + pSrcMatrix2->rows) );
	AUD_ASSERT( pDstMatrix->cols == pSrcMatrix1->cols && pDstMatrix->cols == pSrcMatrix2->cols  );
	AUD_ASSERT( pDstMatrix->dataType == pSrcMatrix1->dataType && pDstMatrix->dataType == pSrcMatrix2->dataType );

	AUD_Int32s firstSize = pSrcMatrix1->rows * pSrcMatrix1->cols * SIZEOF_TYPE( pSrcMatrix1->dataType );

	memmove( pDstMatrix->pInt16s, pSrcMatrix1->pInt16s, firstSize );
	memmove( (AUD_Int8s*)pDstMatrix->pInt16s + firstSize, pSrcMatrix2->pInt16s, pSrcMatrix2->rows * pSrcMatrix2->cols * SIZEOF_TYPE( pSrcMatrix2->dataType ) );

	return 0;
}

AUD_Int32s copyMatrixRow( AUD_Matrix *pDstMatrix, AUD_Int32s dstRow, AUD_Matrix *pSrcMatrix, AUD_Int32s srcRow )
{
	AUD_ASSERT( pDstMatrix && pSrcMatrix );
	AUD_ASSERT( pDstMatrix->dataType == pSrcMatrix->dataType );
	AUD_ASSERT( pDstMatrix->cols == pSrcMatrix->cols );
	AUD_ASSERT( dstRow < pDstMatrix->rows && dstRow >= 0 );
	AUD_ASSERT( srcRow < pSrcMatrix->rows && srcRow >= 0 );

	AUD_Int32s cols = pDstMatrix->cols;

	switch ( pDstMatrix->dataType )
	{
	case AUD_DATATYPE_INT16S:
		memmove( pDstMatrix->pInt16s + dstRow * cols, pSrcMatrix->pInt16s + srcRow * cols, cols * SIZEOF_TYPE( pDstMatrix->dataType ) );
		break;

	case AUD_DATATYPE_INT32S:
		memmove( pDstMatrix->pInt32s + dstRow * cols, pSrcMatrix->pInt32s + srcRow * cols, cols * SIZEOF_TYPE( pDstMatrix->dataType ) );
		break;

	case AUD_DATATYPE_INT64S:
		memmove( pDstMatrix->pInt64s + dstRow * cols, pSrcMatrix->pInt64s + srcRow * cols, cols * SIZEOF_TYPE( pDstMatrix->dataType ) );
		break;

	case AUD_DATATYPE_DOUBLE:
		memmove( pDstMatrix->pDouble + dstRow * cols, pSrcMatrix->pDouble + srcRow * cols, cols * SIZEOF_TYPE( pDstMatrix->dataType ) );
		break;

	default:
		AUD_ASSERT( 0 );
		break;
	}
	return 0;
}

AUD_Int32s transposeMatrix( AUD_Matrix *pSrcMatrix, AUD_Matrix *pDstMatrix )
{
	AUD_ASSERT( pSrcMatrix && pDstMatrix );
	AUD_ASSERT( pSrcMatrix->rows > 0 && pSrcMatrix->cols > 0 );
	AUD_ASSERT( pDstMatrix->rows > 0 && pDstMatrix->cols > 0 );
	AUD_ASSERT( pSrcMatrix->cols == pDstMatrix->rows );
	AUD_ASSERT( pSrcMatrix->rows == pDstMatrix->cols );
	AUD_ASSERT( pSrcMatrix->dataType == pDstMatrix->dataType );

	AUD_Int32s i, j;
	switch ( pSrcMatrix->dataType )
	{
	case AUD_DATATYPE_INT16S:
		for ( i = 0; i < pSrcMatrix->rows; i++ )
		{
			for ( j = 0; j < pSrcMatrix->cols; j++ )
			{
				*( pDstMatrix->pInt16s + j * pDstMatrix->cols + i ) = *( pSrcMatrix->pInt16s + i * pSrcMatrix->cols + j );
			}
		}
		break;

	case AUD_DATATYPE_INT32S:
		for ( i = 0; i < pSrcMatrix->rows; i++ )
		{
			for ( j = 0; j < pSrcMatrix->cols; j++ )
			{
				*( pDstMatrix->pInt32s + j * pDstMatrix->cols + i ) = *( pSrcMatrix->pInt32s + i * pSrcMatrix->cols + j );
			}
		}
		break;

	case AUD_DATATYPE_INT64S:
		for ( i = 0; i < pSrcMatrix->rows; i++ )
		{
			for ( j = 0; j < pSrcMatrix->cols; j++ )
			{
				*( pDstMatrix->pInt64s + j * pDstMatrix->cols + i ) = *( pSrcMatrix->pInt64s + i * pSrcMatrix->cols + j );
			}
		}
		break;

	case AUD_DATATYPE_DOUBLE:
		for ( i = 0; i < pSrcMatrix->rows; i++ )
		{
			for ( j = 0; j < pSrcMatrix->cols; j++ )
			{
				*( pDstMatrix->pDouble + j * pDstMatrix->cols + i ) = *( pSrcMatrix->pDouble + i * pSrcMatrix->cols + j );
			}
		}
		break;

	default:
		AUD_ASSERT( 0 );
		break;
	}
	AUDLOG( "\n" );

	return 0;
}

AUD_Int32s pushCircularMatrix( AUD_CircularMatrix *pCircularMatrix, void *pArray )
{
	AUD_ASSERT( pCircularMatrix && pArray );

	pCircularMatrix->curPosition = ( pCircularMatrix->curPosition + 1 ) % pCircularMatrix->matrix.rows;
	if ( ( pCircularMatrix->curPosition == pCircularMatrix->matrix.rows - 1 ) && ( pCircularMatrix->isFull == AUD_FALSE ) )
	{
		pCircularMatrix->isFull = AUD_TRUE;
	}

	AUD_Int8s *pDst = GET_MATRIX_ELEPOINTER( &(pCircularMatrix->matrix), pCircularMatrix->curPosition, 0 );
	AUD_ASSERT( pDst );
	memcpy( pDst, pArray, SIZEOF_TYPE( pCircularMatrix->matrix.dataType ) * pCircularMatrix->matrix.cols );

	return 0;
}

AUD_Int32s showMatrix( AUD_Matrix *pMatrix )
{
	AUD_ASSERT( pMatrix );
	AUD_Int32s i, j;

	AUDLOG( "###### matrix rows: %d, cols: %d, dataType: %d ######\n", pMatrix->rows, pMatrix->cols, pMatrix->dataType );
	switch ( pMatrix->dataType )
	{
	case AUD_DATATYPE_INT16S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols; j++ )
			{
				AUDLOG( "%d, ", *( pMatrix->pInt16s + i * pMatrix->cols + j ) );
			}
			AUDLOG( "\n" );
		}
		break;

	case AUD_DATATYPE_INT32S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols; j++ )
			{
				AUDLOG( "%d, ", *( pMatrix->pInt32s + i * pMatrix->cols + j ) );
			}
			AUDLOG( "\n" );
		}
		break;

	case AUD_DATATYPE_INT64S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols; j++ )
			{
				AUDLOG( "%lld, ", *( pMatrix->pInt64s + i * pMatrix->cols + j ) );
			}
			AUDLOG( "\n" );
		}
		break;

	case AUD_DATATYPE_DOUBLE:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols; j++ )
			{
				AUDLOG( "%f, ", *( pMatrix->pDouble + i * pMatrix->cols + j ) );
			}
			AUDLOG( "\n" );
		}
		break;

	default:
		AUD_ASSERT( 0 );
		break;
	}
	AUDLOG( "\n" );

	return 0;
}

AUD_Int32s destroyMatrix( AUD_Matrix *pMatrix )
{
	AUD_ASSERT( pMatrix );

	if ( pMatrix->pInt16s == NULL )
	{
		return -1;
	}

	// since union member share the value, do just free one of them is OK
	free( (void*)(pMatrix->pInt16s) );

	pMatrix->pInt16s = NULL;
	pMatrix->rows          = 0;
	pMatrix->cols          = 0;
	pMatrix->dataType      = AUD_DATATYPE_INVALID;

	return 0;
}

AUD_Int32s printMatrixToFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName )
{
	AUD_ASSERT( pMatrix && pFileName );

	FILE *fp = NULL;

	fp = fopen( (const char*)pFileName, "a+" );
	if ( !fp )
	{
		AUDLOG( "open file %s for write failed", pFileName );
		return AUD_ERROR_IOFAILED;
	}

	AUD_Int32s i, j;

	AUDLOG( "###### matrix rows: %d, cols: %d, dataType: %d ######\n", pMatrix->rows, pMatrix->cols, pMatrix->dataType );
	switch ( pMatrix->dataType )
	{
	case AUD_DATATYPE_INT16S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols - 1; j++ )
			{
				fprintf( fp, "%d, ", *( pMatrix->pInt16s + i * pMatrix->cols + j ) );
			}
			fprintf( fp, "%d\n", *( pMatrix->pInt16s + i * pMatrix->cols + j ) );
		}
		break;

	case AUD_DATATYPE_INT32S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols - 1; j++ )
			{
				fprintf( fp, "%d, ", *( pMatrix->pInt32s + i * pMatrix->cols + j ) );
			}
			fprintf( fp, "%d\n", *( pMatrix->pInt32s + i * pMatrix->cols + j ) );
		}
		break;

	case AUD_DATATYPE_INT64S:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols - 1; j++ )
			{
				fprintf( fp, "%lld, ", *( pMatrix->pInt64s + i * pMatrix->cols + j ) );
			}
			fprintf( fp, "%lld\n", *( pMatrix->pInt64s + i * pMatrix->cols + j ) );
		}
		break;

	case AUD_DATATYPE_DOUBLE:
		for ( i = 0; i < pMatrix->rows; i++ )
		{
			for ( j = 0; j < pMatrix->cols - 1; j++ )
			{
				fprintf( fp, "%f, ", *( pMatrix->pDouble + i * pMatrix->cols + j ) );
			}
			fprintf( fp, "%f\n", *( pMatrix->pDouble + i * pMatrix->cols + j ) );
		}
		break;

	default:
		AUD_ASSERT( 0 );
		break;
	}

	return 0;
}


AUD_Int32s writeMatrixToFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName )
{
	AUD_ASSERT( pMatrix && pFileName );

	FILE *fp = NULL;

	fp = fopen( (const char*)pFileName, "wb" );
	if ( !fp )
	{
		AUDLOG( "open file %s for write failed", pFileName );
		return AUD_ERROR_IOFAILED;
	}

	(void)fwrite( &( pMatrix->dataType ), sizeof( pMatrix->dataType ), 1, fp );
	(void)fwrite( &( pMatrix->rows ), sizeof( pMatrix->rows ), 1, fp );
	(void)fwrite( &( pMatrix->cols ), sizeof( pMatrix->cols ), 1, fp );

	(void)fwrite( (const void*)( pMatrix->pInt16s ), SIZEOF_TYPE( pMatrix->dataType ) * pMatrix->rows * pMatrix->cols, 1, fp );

	fclose( fp );
	fp = NULL;

	return 0;
}

// XXX: bad practice, betray memory management principle, need refine
AUD_Int32s readMatrixFromFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName )
{
	AUD_ASSERT( pMatrix && pFileName );

	FILE       *fp = NULL;
	AUD_Int32s ret = 0, data;

	fp = fopen( (const char*)pFileName, "rb" );
	if ( !fp )
	{
		AUDLOG( "open file %s for read failed", pFileName );
		return AUD_ERROR_IOFAILED;
	}

	data = fread( &( pMatrix->dataType ), sizeof( pMatrix->dataType ), 1, fp );
	data = fread( &( pMatrix->rows ), sizeof( pMatrix->rows ), 1, fp );
	data = fread( &( pMatrix->cols ), sizeof( pMatrix->cols ), 1, fp );

	ret = createMatrix( pMatrix );
	AUD_ASSERT( ret == 0 );

	data = fread( (void*)( pMatrix->pInt16s ), SIZEOF_TYPE( pMatrix->dataType  ) * pMatrix->rows * pMatrix->cols, 1, fp );

	fclose( fp );
	fp = NULL;

	return 0;
}

// algebra operations

/* mean & variance normalization of matrix, along specific dimension( 1: row; 2: col ) */
AUD_Int32s mvnMatrix( AUD_Matrix *pMatrix, AUD_Int32s rowOrCol )
{
	AUD_ASSERT( pMatrix );
	AUD_ASSERT( pMatrix->dataType == AUD_DATATYPE_INT32S );

	AUD_Int32s normalizeStep, normalizeStride, normalizeVectors, normalizeLen;
	if ( rowOrCol == 1 )
	{
		normalizeStep    = pMatrix->cols;
		normalizeStride  = 1;
		normalizeVectors = pMatrix->rows;
		normalizeLen     = pMatrix->cols;
	}
	else if ( rowOrCol == 2 )
	{
		normalizeStep    = 1;
		normalizeStride  = pMatrix->cols;
		normalizeVectors = pMatrix->cols;
		normalizeLen     = pMatrix->rows;
	}
	else
	{
		AUD_ASSERT( 0 );
	}

	AUD_Double mean = 0., var = 0., stdvar = 0.;
	AUD_Int32s *pData = pMatrix->pInt32s;
	AUD_Int32s i, j;
	for ( i = 0; i < normalizeVectors; i++ )
	{
		// calc mean
		mean = 0.;
		for ( j = 0; j < normalizeLen; j++ )
		{
			mean += pData[i * normalizeStep + j * normalizeStride];
		}
		mean /= normalizeLen;

		// calc variance
		var = 0.;
		for ( j = 0; j < normalizeLen; j++ )
		{
			var += pow( (pData[i * normalizeStep + j * normalizeStride] - mean), 2.0 );
		}
		var /= normalizeLen;
		stdvar = sqrt( var );

		// normalize
		AUD_Double tmp = 0.;
		for ( j = 0; j < normalizeLen; j++ )
		{
			tmp = (pData[i * normalizeStep + j * normalizeStride] - mean) / stdvar;
			pData[i * normalizeStep + j * normalizeStride] = (AUD_Int32s)round( tmp * 32768.0 );
		}
	}

	return 0;
}

AUD_Int32s multiplyMatrix( AUD_Matrix *pLeftMatrix, AUD_Matrix *pRightMatrix, AUD_Matrix *pDstMatrix )
{
	AUD_ASSERT( pLeftMatrix && pRightMatrix && pDstMatrix );
	AUD_ASSERT( pLeftMatrix->rows > 0 && pLeftMatrix->cols > 0 );
	AUD_ASSERT( pRightMatrix->rows > 0 && pRightMatrix->cols > 0 );
	AUD_ASSERT( pDstMatrix->rows > 0 && pDstMatrix->cols > 0 );
	AUD_ASSERT( pLeftMatrix->cols == pRightMatrix->rows );
	AUD_ASSERT( pLeftMatrix->rows == pDstMatrix->rows );
	AUD_ASSERT( pRightMatrix->cols == pDstMatrix->cols );

	if ( pLeftMatrix->dataType == AUD_DATATYPE_DOUBLE && pRightMatrix->dataType == AUD_DATATYPE_INT32S && pDstMatrix->dataType == AUD_DATATYPE_DOUBLE )
	{
		AUD_Int32s i, j, k;
		AUD_Double *pLeft  = pLeftMatrix->pDouble;
		AUD_Int32s *pRight = pRightMatrix->pInt32s;
		AUD_Double *pDst   = pDstMatrix->pDouble;
		for ( i = 0; i < pLeftMatrix->rows; i++ )
		{
			AUD_Double *pLeftRow = pLeft + i * pLeftMatrix->cols;
			AUD_Double *pDstRow  = pDst + i * pDstMatrix->cols;
			for ( k = 0; k < pRightMatrix->cols; k++ )
			{
				AUD_Double suma = 0.;
				for ( j = 0; j < pLeftMatrix->cols; j++ )
				{
					suma += pLeftRow[j] * ( *(pRight + j * pRightMatrix->cols + k) );
				}
				pDstRow[k] = suma;
			}
		}
	}
	else if ( pLeftMatrix->dataType == AUD_DATATYPE_DOUBLE && pRightMatrix->dataType == AUD_DATATYPE_DOUBLE && pDstMatrix->dataType == AUD_DATATYPE_DOUBLE )
	{
		AUD_Int32s i, j, k;
		AUD_Double *pLeft  = pLeftMatrix->pDouble;
		AUD_Double *pRight = pRightMatrix->pDouble;
		AUD_Double *pDst   = pDstMatrix->pDouble;
		for ( i = 0; i < pLeftMatrix->rows; i++ )
		{
			AUD_Double *pLeftRow = pLeft + i * pLeftMatrix->cols;
			AUD_Double *pDstRow  = pDst + i * pDstMatrix->cols;
			for ( k = 0; k < pRightMatrix->cols; k++ )
			{
				AUD_Double suma = 0.;
				for ( j = 0; j < pLeftMatrix->cols; j++ )
				{
					suma += pLeftRow[j] * ( *(pRight + j * pRightMatrix->cols + k) );
				}
				pDstRow[k] = suma;
			}
		}
	}
	else if ( pLeftMatrix->dataType == AUD_DATATYPE_INT32S && pRightMatrix->dataType == AUD_DATATYPE_FLOAT && pDstMatrix->dataType == AUD_DATATYPE_INT32S )
	{
		AUD_Int32s i, j, k;
		AUD_Int32s *pLeft  = pLeftMatrix->pInt32s;
		AUD_Float  *pRight = pRightMatrix->pFloat;
		AUD_Int32s *pDst   = pDstMatrix->pInt32s;
		for ( i = 0; i < pLeftMatrix->rows; i++ )
		{
			AUD_Int32s *pLeftRow = pLeft + i * pLeftMatrix->cols;
			AUD_Int32s *pDstRow  = pDst + i * pDstMatrix->cols;
			for ( k = 0; k < pRightMatrix->cols; k++ )
			{
				AUD_Float suma = 0.;
				for ( j = 0; j < pLeftMatrix->cols; j++ )
				{
					suma += pLeftRow[j] * ( *(pRight + j * pRightMatrix->cols + k) );
				}
				pDstRow[k] = (AUD_Int32s)round( suma );
			}
		}
	}
	else
	{
		AUD_ASSERT( 0 );
	}

	return 0;
}

// vector
AUD_Int32s createVector( AUD_Vector *pVector )
{
	AUD_ASSERT( pVector && pVector->len > 0 );

	// since union member share the value, do just allocate one of them is OK
	pVector->pInt16s = (AUD_Int16s*)calloc( pVector->len * SIZEOF_TYPE( pVector->dataType ), 1 );
	if ( pVector->pInt16s == NULL )
	{
		return -1;
	}

	return 0;
}

AUD_Int32s cloneVector( AUD_Vector *pDstVector, AUD_Vector *pSrcVector )
{
	AUD_ASSERT( pDstVector && pSrcVector );
	AUD_ASSERT( pDstVector->len == pSrcVector->len && pSrcVector->len > 0 );
	AUD_ASSERT( pDstVector->dataType == pSrcVector->dataType );

	memmove( pDstVector->pInt16s, pSrcVector->pInt16s, pDstVector->len * SIZEOF_TYPE( pDstVector->dataType ) );

	return 0;
}

AUD_Int32s mergeVector( AUD_Vector *pDstVector, AUD_Vector *pSrcVector1, AUD_Vector *pSrcVector2 )
{
	AUD_ASSERT( pDstVector && pSrcVector1 && pSrcVector2 );
	AUD_ASSERT( pDstVector->len == ( pSrcVector1->len + pSrcVector2->len ) );
	AUD_ASSERT( pSrcVector1->len > 0 && pSrcVector2->len > 0 );
	AUD_ASSERT( pDstVector->dataType == pSrcVector1->dataType && pDstVector->dataType == pSrcVector2->dataType );

	AUD_Int32s firstSize = pSrcVector1->len * SIZEOF_TYPE( pSrcVector1->dataType );
	memmove( pDstVector->pInt16s, pSrcVector1->pInt16s, firstSize );
	memmove( (AUD_Int8s*)pDstVector->pInt16s + firstSize, pSrcVector2->pInt16s, pSrcVector2->len * SIZEOF_TYPE( pSrcVector2->dataType ) );

	return 0;
}

AUD_Int32s sortVector( AUD_Vector *pVector, AUD_Vector *pSortedIdx )
{
	AUD_ASSERT( pVector && pSortedIdx );
	AUD_ASSERT( pSortedIdx->len <= pVector->len && pSortedIdx->len > 0 );
	AUD_ASSERT( pVector->dataType == AUD_DATATYPE_DOUBLE );
	AUD_ASSERT( pSortedIdx->dataType == AUD_DATATYPE_INT32S );
	AUD_ASSERT( pVector->dataType == AUD_DATATYPE_DOUBLE );

	AUD_Int32s i, j, k;

	pSortedIdx->pInt32s[0] = 0;
	for ( i = 1; i < pVector->len; i++ )
	{
		for ( j = 0; j < AUD_MIN( pSortedIdx->len, i ); j++ )
		{
			if ( pVector->pDouble[i] > pVector->pDouble[pSortedIdx->pInt32s[j]] )
			{
				for ( k = pSortedIdx->len - 2; k >= j; k-- )
				{
					pSortedIdx->pInt32s[k + 1] = pSortedIdx->pInt32s[k];
				}
				pSortedIdx->pInt32s[j] = i;
				break;
			}
		}

		if ( j == i && j < pSortedIdx->len )
		{
			pSortedIdx->pInt32s[j] = i;
		}
	}

#if 0
	AUDLOG( "\n" );
	for ( i = 0; i < pSortedIdx->len; i++ )
	{
		AUDLOG( "%f, ", pVector->pDouble[pSortedIdx->pInt32s[i]] );
	}
	AUDLOG( "\n" );
#endif

	return 0;
}

AUD_Int32s sortVectorWithPercent( AUD_Vector *pVector, AUD_Vector *pSortedIdx, AUD_Double percent, AUD_Int32s *pSortNum )
{
	AUD_ASSERT( pVector && pSortedIdx && pSortNum );
	AUD_ASSERT( pSortedIdx->len == pVector->len && pSortedIdx->len > 0 );
	AUD_ASSERT( pVector->dataType == AUD_DATATYPE_DOUBLE );
	AUD_ASSERT( pSortedIdx->dataType == AUD_DATATYPE_INT32S );
	AUD_ASSERT( pVector->dataType == AUD_DATATYPE_DOUBLE );
	AUD_ASSERT( percent > 0 && percent <= 1 );

	AUD_Int32s i, j, k;
	AUD_Double sum = 0;

	pSortedIdx->pInt32s[0] = 0;
	for ( i = 1; i < pSortedIdx->len; i++ )
	{
		for ( j = 0; j < pVector->len; j++ )
		{
			if ( pVector->pDouble[i] > pVector->pDouble[pSortedIdx->pInt32s[j]] )
			{
				for ( k = pSortedIdx->len - 2; k >= j; k-- )
				{
					pSortedIdx->pInt32s[k + 1] = pSortedIdx->pInt32s[k];
				}
				pSortedIdx->pInt32s[j] = i;
				break;
			}
		}

		if ( j == i && j < pSortedIdx->len )
		{
			pSortedIdx->pInt32s[j] = i;
		}
	}

	sum = 0;
	for ( i = 0; i < pSortedIdx->len; i++ )
	{
		sum += pVector->pDouble[pSortedIdx->pInt32s[i]];
	}

	AUD_Double partialSum = sum;
	for ( i = pSortedIdx->len - 1; i >= 0; i++ )
	{
		partialSum -= pVector->pDouble[pSortedIdx->pInt32s[i]];
		if ( partialSum / sum <= percent )
		{
			break;
		}
	}

	*pSortNum = i + 1;

#if 0
	AUDLOG( "\n" );
	for ( i = 0; i < pSortedIdx->len; i++ )
	{
		AUDLOG( "%f, ", pVector->pDouble[pSortedIdx->pInt32s[i]] );
	}
	AUDLOG( "\n" );
#endif

	return 0;
}

AUD_Int32s showVector( AUD_Vector *pVector )
{
	AUD_ASSERT( pVector );

	AUDLOG( "###### vector len: %d, dataType: %d ######\n", pVector->len, pVector->dataType );
	switch ( pVector->dataType )
	{
	case AUD_DATATYPE_INT16S:
		for ( AUD_Int32s i = 0; i < pVector->len; i++ )
		{
			AUDLOG( "%d, ", *( pVector->pInt16s + i ) );
		}
		AUDLOG( "\n" );
		break;

	case AUD_DATATYPE_INT32S:
		for ( AUD_Int32s i = 0; i < pVector->len; i++ )
		{
			AUDLOG( "%d, ", *( pVector->pInt32s + i ) );
		}
		AUDLOG( "\n" );
		break;

	case AUD_DATATYPE_INT64S:
		for ( AUD_Int32s i = 0; i < pVector->len; i++ )
		{
			AUDLOG( "%lld, ", *( pVector->pInt64s + i ) );
		}
		AUDLOG( "\n" );
		break;

	case AUD_DATATYPE_DOUBLE:
		for ( AUD_Int32s i = 0; i < pVector->len; i++ )
		{
			AUDLOG( "%f, ", *( pVector->pDouble + i ) );
		}
		AUDLOG( "\n" );
		break;

	default:
		AUD_ASSERT( 0 );
		break;
	}
	AUDLOG( "\n" );

	return 0;
}

AUD_Int32s destroyVector( AUD_Vector *pVector )
{
	AUD_ASSERT( pVector );

	if ( pVector->pInt16s == NULL )
	{
		return -1;
	}

	// since union member share the value, do just free one of them is OK
	free( (void*)(pVector->pInt16s) );

	pVector->pInt16s  = NULL;
	pVector->len      = 0;
	pVector->dataType = AUD_DATATYPE_INVALID;

	return 0;
}
