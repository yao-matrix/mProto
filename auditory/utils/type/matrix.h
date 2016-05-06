/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _MATRIX_H_ )
#define _MATRIX_H_

#include "AudioDef.h"

// matrix
typedef struct
{
	union
	{
		AUD_Int16s  *pInt8s;
		AUD_Int16s  *pInt16s;
		AUD_Int32s  *pInt32s;
		AUD_Int64s  *pInt64s;
		AUD_Float   *pFloat;
		AUD_Double  *pDouble;
	};
	AUD_Int32s    rows;
	AUD_Int32s    cols;
	AUD_DataType  dataType;
} AUD_Matrix;

typedef struct
{
	AUD_Matrix    matrix;
	AUD_Int32s    curPosition;
	AUD_Bool      isFull;
} AUD_CircularMatrix;


#define GET_MATRIX_ELEPOINTER( pMatrix, row , col )\
        ( ( (pMatrix)->dataType == AUD_DATATYPE_INT16S ) ?\
          (AUD_Int8s*)((pMatrix)->pInt16s + (row) * (pMatrix)->cols + (col)) :\
          ( ( (pMatrix)->dataType == AUD_DATATYPE_INT32S ) ?\
            (AUD_Int8s*)( (pMatrix)->pInt32s + (row) * (pMatrix)->cols + (col) ) :\
            ( ( (pMatrix)->dataType == AUD_DATATYPE_INT8S ) ?\
              (AUD_Int8s*)( (pMatrix)->pInt8s + (row) * (pMatrix)->cols + (col) ) :\
              ( ( (pMatrix)->dataType == AUD_DATATYPE_FLOAT ) ?\
                (AUD_Int8s*)( (pMatrix)->pFloat + (row) * (pMatrix)->cols + (col) ) :\
                ( ( (pMatrix)->dataType == AUD_DATATYPE_DOUBLE ) ?\
                  (AUD_Int8s*)( (pMatrix)->pDouble + (row) * (pMatrix)->cols + (col) ) :\
                  NULL\
                )\
              )\
            )\
          )\
        )


#define GET_MATRIX_ELEMENT( pMatrix, row , col )\
        ( ( (pMatrix)->dataType == AUD_DATATYPE_INT16S ) ?\
          *((pMatrix)->pInt16s + row * (pMatrix)->cols + (col) ) :\
          ( ( (pMatrix)->dataType == AUD_DATATYPE_INT32S ) ?\
            *((pMatrix)->pInt32s + row * (pMatrix)->cols + (col) ) :\
            ( ( (pMatrix)->dataType == AUD_DATATYPE_INT8S ) ?\
              *( (pMatrix)->pInt8s + (row) * (pMatrix)->cols + (col) ) :\
              ( ( (pMatrix)->dataType == AUD_DATATYPE_FLOAT ) ?\
                *( (pMatrix)->pFloat + (row) * (pMatrix)->cols + (col) ) :\
                ( ( (pMatrix)->dataType == AUD_DATATYPE_DOUBLE ) ?\
                  *( (pMatrix)->pDouble + (row) * (pMatrix)->cols + (col) ) :\
                  -1\
                )\
              )\
            )\
          )\
        )


AUD_Int32s createMatrix( AUD_Matrix *pMatrix );

AUD_Int32s cleanMatrix( AUD_Matrix *pMatrix );

AUD_Int32s copyMatrixRow( AUD_Matrix *pDstMatrix, AUD_Int32s dstRow, AUD_Matrix *pSrcMatrix, AUD_Int32s srcRow );

AUD_Int32s cloneMatrix( AUD_Matrix *pDstMatrix,  AUD_Matrix *pSrcMatrix );

AUD_Int32s transposeMatrix( AUD_Matrix *pSrcMatrix, AUD_Matrix *pDstMatrix );

AUD_Int32s mergeMatrix( AUD_Matrix *pDstMatrix,  AUD_Matrix *pSrcMatrix1, AUD_Matrix *pSrcMatrix2 );

AUD_Int32s pushCircularMatrix( AUD_CircularMatrix *pCircularMatrix, void *pArray );

AUD_Int32s showMatrix( AUD_Matrix *pMatrix );

AUD_Int32s destroyMatrix( AUD_Matrix *pMatrix );

// matrix file read/write
AUD_Int32s printMatrixToFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName );

AUD_Int32s writeMatrixToFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName );

AUD_Int32s readMatrixFromFile( AUD_Matrix *pMatrix, AUD_Int8s *pFileName );

// matrix algebra operations
AUD_Int32s multiplyMatrix( AUD_Matrix *pLeftMatrix, AUD_Matrix *pRightMatrix, AUD_Matrix *pDstMatrix );

AUD_Int32s mvnMatrix( AUD_Matrix *pMatrix, AUD_Int32s rowOrCol );


// vector
typedef struct
{
	union
	{
		AUD_Int16s  *pInt8s;
		AUD_Int16s  *pInt16s;
		AUD_Int32s  *pInt32s;
		AUD_Int64s  *pInt64s;
		AUD_Int64s  *pFloat;
		AUD_Double  *pDouble;
	};
	AUD_Int32s    len;
	AUD_DataType  dataType;
} AUD_Vector;

AUD_Int32s createVector( AUD_Vector *pVector );

AUD_Int32s cloneVector( AUD_Vector *pDstVector, AUD_Vector *pSrcVector );

AUD_Int32s mergeVector( AUD_Vector *pDstVector, AUD_Vector *pSrcVector1, AUD_Vector *pSrcVector2 );

AUD_Int32s sortVector( AUD_Vector *pVector, AUD_Vector *pSortedIdx );

AUD_Int32s sortVectorWithPercent( AUD_Vector *pVector, AUD_Vector *pSortedIdx, AUD_Double percent, AUD_Int32s *pSortNum );

AUD_Int32s showVector( AUD_Vector *pVector );

AUD_Int32s destroyVector( AUD_Vector *pVector );

#endif // _MATRIX_H_
