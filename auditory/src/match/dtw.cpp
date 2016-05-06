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
#include "type/matrix.h"

#define DTW_DEBUG 0

#if DTW_DEBUG
static FILE *fp = NULL;
#endif

// forward declaration
static AUD_Error _getPath( AUD_DTWSession *pDTWSession, AUD_Int32s arStartCell[2], const AUD_Int32s arGoalCell[2], AUD_Double *pWarpPathLen, AUD_Vector *pPath );
static AUD_Error _computeGoalCell( AUD_Matrix *pDTW, AUD_Int32s validCols, AUD_Int32s arGoalCell[2] );


AUD_Error dtw_initsession( AUD_DTWSession *pDTWSession, AUD_Feature *pTemplate, AUD_Int32s inputFeatureNum )
{
	AUD_ASSERT( pDTWSession && pTemplate && ( inputFeatureNum > 0 ) );

	AUD_Int32s ret;
	AUD_Int32s patternSize;

	pDTWSession->pTemplate = pTemplate;

	patternSize = pDTWSession->pTemplate->featureMatrix.rows;

	// create frame cost matrix
	pDTWSession->frameCost.matrix.rows     = inputFeatureNum;
	pDTWSession->frameCost.matrix.cols     = patternSize;
	pDTWSession->frameCost.matrix.dataType = AUD_DATATYPE_DOUBLE;
	ret = createMatrix( &(pDTWSession->frameCost.matrix) );
	AUD_ASSERT( ret == 0 );

	pDTWSession->frameCost.curPosition = -1;
	pDTWSession->frameCost.isFull      = AUD_FALSE;

	// create dtw matrix
	pDTWSession->dtw.rows     = patternSize;
	pDTWSession->dtw.cols     = inputFeatureNum;
	pDTWSession->dtw.dataType = AUD_DATATYPE_DOUBLE;
	ret = createMatrix( &(pDTWSession->dtw) );
	AUD_ASSERT( ret == 0 );

	// create work buffer
	pDTWSession->pWorkBuf = (AUD_Double*)malloc( pDTWSession->frameCost.matrix.cols * sizeof(AUD_Double) );
	AUD_ASSERT( pDTWSession->pWorkBuf );

#if DTW_DEBUG
	fp = fopen( "./dtw_cost.log", "ab+" );
	// fp = stdout;
	AUD_ASSERT( fp );
#endif

	return AUD_ERROR_NONE;
}

AUD_Error dtw_deinitsession( AUD_DTWSession *pDTWSession )
{
	AUD_ASSERT( pDTWSession );

	AUD_Int32s ret;

	// destroy frame cost matrix
	ret = destroyMatrix( &(pDTWSession->frameCost.matrix) );
	AUD_ASSERT( ret == 0 );

	// destroy dtw matrix
	ret = destroyMatrix( &(pDTWSession->dtw) );
	AUD_ASSERT( ret == 0 );

	// free work buffer
	free( pDTWSession->pWorkBuf );
	pDTWSession->pWorkBuf = NULL;

#if DTW_DEBUG
	fclose( fp );
	fp = NULL;
#endif

	return AUD_ERROR_NONE;
}

AUD_Error dtw_refreshfrmcost( AUD_DTWSession *pDTWSession )
{
	AUD_ASSERT( pDTWSession );

	pDTWSession->frameCost.curPosition = -1;
	pDTWSession->frameCost.isFull      = AUD_FALSE;

	return AUD_ERROR_NONE;
}

AUD_Error dtw_updatefrmcost( AUD_DTWSession *pDTWSession, AUD_Feature *pNewFeature )
{
	AUD_ASSERT( pDTWSession && pNewFeature );

	AUD_Int32s i, j, k;
	AUD_Int32s vecDim = pDTWSession->pTemplate->featureMatrix.cols;

#if 0
	AUDLOG( "in feature matrix\n" );
	for ( i = 0; i < pNewFeature->featureMatrix.rows; i++ )
	{
		for ( j = 0; j < pNewFeature->featureMatrix.cols; j++ )
		{
			AUDLOG( "%d, ", *(pNewFeature->featureMatrix.pInt32s + i * pNewFeature->featureMatrix.cols + j) );
		}
		AUDLOG( "norm: %lld\n", pNewFeature->featureNorm.pInt64s[i] );
	}
	AUDLOG( "\n\n" );

	AUDLOG( "template feature matrix\n" );
	for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
	{
		for ( j = 0; j < pDTWSession->pTemplate->featureMatrix.cols; j++ )
		{
			AUDLOG( "%d, ", *(pDTWSession->pTemplate->featureMatrix.pInt32s + i * pDTWSession->pTemplate->featureMatrix.cols + j) );
		}
		AUDLOG( "norm: %lld\n", pDTWSession->pTemplate->featureNorm.pInt64s[i] );
	}
	AUDLOG( "\n\n" );
#endif

	if ( pDTWSession->pTemplate->featureMatrix.dataType == AUD_DATATYPE_INT32S )
	{
		if ( pDTWSession->distType == AUD_DISTTYPE_COSINE )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)((pNewFeature->featureNorm.pInt64s)[k]);
				AUD_Int32s *pFeature    = pNewFeature->featureMatrix.pInt32s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)((pDTWSession->pTemplate->featureNorm.pInt64s)[i]);
					AUD_Int32s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt32s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += (AUD_Double)pTemplateFeature[j] * (AUD_Double)pFeature[j];
					}

					pDTWSession->pWorkBuf[i] = tmp / templateNorm / inNorm;

					// AUDLOG( "%.2f : %.2f : %.2f : %.2f, ", tmp, templateNorm, inNorm, pDTWSession->pWorkBuf[i] );

					pDTWSession->pWorkBuf[i] = 1 - pDTWSession->pWorkBuf[i];
				}
				// AUDLOG( "\n" );

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else if ( pDTWSession->distType == AUD_DISTTYPE_EUCLIDEAN )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)(pNewFeature->featureNorm.pInt64s)[k];
				AUD_Int32s *pFeature    = pNewFeature->featureMatrix.pInt32s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)(pDTWSession->pTemplate->featureNorm.pInt64s)[i];
					AUD_Int32s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt32s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += pow( (AUD_Double)pTemplateFeature[j] / templateNorm - (AUD_Double)pFeature[j] / inNorm, 2. );
					}

					pDTWSession->pWorkBuf[i] = sqrt( tmp );
				}

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else if ( pDTWSession->distType == AUD_DISTTYPE_MANHATTAN )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)(pNewFeature->featureNorm.pInt64s)[k];
				AUD_Int32s *pFeature    = pNewFeature->featureMatrix.pInt32s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)(pDTWSession->pTemplate->featureNorm.pInt64s)[i];
					AUD_Int32s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt32s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += fabs( (AUD_Double)pTemplateFeature[j] / templateNorm - (AUD_Double)pFeature[j] / inNorm );
					}

					pDTWSession->pWorkBuf[i] = tmp;
				}

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else
		{
			AUD_ASSERT( 0 );
		}
	}
	else if ( pDTWSession->pTemplate->featureMatrix.dataType == AUD_DATATYPE_INT16S )
	{
		if ( pDTWSession->distType == AUD_DISTTYPE_COSINE )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)(pNewFeature->featureNorm.pInt64s)[k];
				AUD_Int16s *pFeature    = pNewFeature->featureMatrix.pInt16s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)(pDTWSession->pTemplate->featureNorm.pInt64s)[i];
					AUD_Int16s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt16s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += (AUD_Double)pTemplateFeature[j] * (AUD_Double)pFeature[j];
					}

					pDTWSession->pWorkBuf[i] = tmp / templateNorm / inNorm;

					pDTWSession->pWorkBuf[i] = 1 - pDTWSession->pWorkBuf[i];
				}

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else if ( pDTWSession->distType == AUD_DISTTYPE_EUCLIDEAN )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)(pNewFeature->featureNorm.pInt64s)[k];
				AUD_Int16s *pFeature    = pNewFeature->featureMatrix.pInt16s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)(pDTWSession->pTemplate->featureNorm.pInt64s)[i];
					AUD_Int16s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt16s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += pow( (AUD_Double)pTemplateFeature[j] / templateNorm - (AUD_Double)pFeature[j] / inNorm, 2. );
					}

					pDTWSession->pWorkBuf[i] = sqrt( tmp );
				}

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else if ( pDTWSession->distType == AUD_DISTTYPE_MANHATTAN )
		{
			for ( k = 0; k < pNewFeature->featureMatrix.rows; k++ )
			{
				AUD_Double inNorm       = (AUD_Double)(pNewFeature->featureNorm.pInt64s)[k];
				AUD_Int16s *pFeature    = pNewFeature->featureMatrix.pInt16s + k * vecDim;
				for ( i = 0; i < pDTWSession->pTemplate->featureMatrix.rows; i++ )
				{
					AUD_Double tmp               = 0.;
					AUD_Double templateNorm      = (AUD_Double)(pDTWSession->pTemplate->featureNorm.pInt64s)[i];
					AUD_Int16s *pTemplateFeature = pDTWSession->pTemplate->featureMatrix.pInt16s + i * vecDim;

					for ( j = 0; j < vecDim; j++ )
					{
						tmp += fabs( (AUD_Double)pTemplateFeature[j] / templateNorm - (AUD_Double)pFeature[j] / inNorm );
					}

					pDTWSession->pWorkBuf[i] = tmp;
				}

				// push into frame cost matrix
				pushCircularMatrix( &(pDTWSession->frameCost), pDTWSession->pWorkBuf );
			}
		}
		else
		{
			AUD_ASSERT( 0 );
		}
	}
	else
	{
		AUD_ASSERT( 0 );
	}


	return AUD_ERROR_NONE;
}

#define MATCH  0
#define INSERT 1
#define DELETE 2

AUD_Error dtw_match( AUD_DTWSession *pDTWSession, AUD_DTWScoring scoringMethod, AUD_Double *pScore, AUD_Vector *pPath )
{
	AUD_ASSERT( pDTWSession && pScore );
	AUD_ASSERT( !pPath || ( pPath && (pPath->dataType == AUD_DATATYPE_INT32S) && (pPath->len == pDTWSession->dtw.rows) ) );

	AUD_Int32s index;
	AUD_Double arOptions[3];
	AUD_Error  error = AUD_ERROR_NONE;

	/* calc accumulated cost matrix: dtw */
	AUD_Int32s i, j;
	AUD_Int32s validCol = 0;

	// dtw valid col
	// AUDLOG( "frame cost matrix current position: %d\n", pDTWSession->frameCost.curPosition );
	if ( pDTWSession->frameCost.isFull == AUD_TRUE )
	{
		validCol = pDTWSession->frameCost.matrix.rows;
		index    = ( pDTWSession->frameCost.curPosition + 1 ) % pDTWSession->frameCost.matrix.rows;
	}
	else
	{
		validCol = pDTWSession->frameCost.curPosition + 1;
		index    = 0;
	}

	AUD_Double currentCost = 0.;

	if ( pDTWSession->transitionType == AUD_DTWTRANSITION_LEVENSHTEIN )
	{
		// row 0
		*(pDTWSession->dtw.pDouble) = *( pDTWSession->frameCost.matrix.pDouble + index * pDTWSession->frameCost.matrix.cols );
		for ( j = 1; j < pDTWSession->dtw.cols; j++ )
		{
			currentCost = *( pDTWSession->frameCost.matrix.pDouble + ( (index + j) % pDTWSession->frameCost.matrix.rows ) * pDTWSession->frameCost.matrix.cols );

			if ( pDTWSession->dtwType == AUD_DTWTYPE_CLASSIC )
			{
				*( pDTWSession->dtw.pDouble + j ) = *( pDTWSession->dtw.pDouble + j - 1 ) + currentCost;
			}
			else if ( pDTWSession->dtwType == AUD_DTWTYPE_SUBSEQUENCE )
			{
				*( pDTWSession->dtw.pDouble + j ) = currentCost;
			}
			else
			{
				AUD_ASSERT( 0 );
			}
		}

		// coloum 0
		for ( i = 1; i < pDTWSession->dtw.rows; i++ )
		{
			*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols ) = *( pDTWSession->dtw.pDouble + (i - 1) * pDTWSession->dtw.cols ) +
                                                                        *( pDTWSession->frameCost.matrix.pDouble + index * pDTWSession->frameCost.matrix.cols + i );
		}

		for ( i = 1; i < pDTWSession->dtw.rows; i++ )
		{
			for ( j = 1; j < pDTWSession->dtw.cols; j++ )
			{
				if ( j < validCol )
				{
					currentCost = *( pDTWSession->frameCost.matrix.pDouble + ( (index + j) % pDTWSession->frameCost.matrix.rows ) * pDTWSession->frameCost.matrix.cols + i );

					arOptions[MATCH]  = *( pDTWSession->dtw.pDouble + (i - 1) * pDTWSession->dtw.cols + j - 1 ) + currentCost;
					arOptions[INSERT] = *( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j - 1 ) + currentCost;
					arOptions[DELETE] = *( pDTWSession->dtw.pDouble + (i - 1) * pDTWSession->dtw.cols + j ) + currentCost;

					*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j ) = AUD_MIN( arOptions[MATCH], AUD_MIN( arOptions[INSERT], arOptions[DELETE] ) );
				}
				else
				{
					*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j ) = 500.0f;
				}
			}
		}
	}
	else if ( pDTWSession->transitionType == AUD_DTWTRANSITION_DTW )
	{
		// row 0
		*(pDTWSession->dtw.pDouble) = *( pDTWSession->frameCost.matrix.pDouble + index * pDTWSession->frameCost.matrix.cols );
		for ( j = 1; j < pDTWSession->dtw.cols; j++ )
		{
			currentCost = *( pDTWSession->frameCost.matrix.pDouble + ( (index + j) % pDTWSession->frameCost.matrix.rows ) * pDTWSession->frameCost.matrix.cols );

			if ( pDTWSession->dtwType == AUD_DTWTYPE_CLASSIC )
			{
				*( pDTWSession->dtw.pDouble + j ) = *( pDTWSession->dtw.pDouble + j - 1 ) + currentCost;
			}
			else if ( pDTWSession->dtwType == AUD_DTWTYPE_SUBSEQUENCE )
			{
				*( pDTWSession->dtw.pDouble + j ) = currentCost;
			}
			else
			{
				AUD_ASSERT( 0 );
			}
		}

		// coloum 0
		for ( i = 1; i < pDTWSession->dtw.rows; i++ )
		{
			// *( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols ) = *( pDTWSession->frameCost.matrix.pDouble + index * pDTWSession->frameCost.matrix.cols + i );
			*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols ) = 500.;
		}

		// row 1
		AUD_Double *pRow0 = pDTWSession->dtw.pDouble;
		AUD_Double *pRow1 = pDTWSession->dtw.pDouble + pDTWSession->dtw.cols;
		for ( j = 1; j < pDTWSession->dtw.cols; j++ )
		{
			currentCost = *( pDTWSession->frameCost.matrix.pDouble + ( (index + j) % pDTWSession->frameCost.matrix.rows ) * pDTWSession->frameCost.matrix.cols + 1 );

			pRow1[j] = AUD_MIN( pRow0[j - 1], pRow1[j - 1] ) + currentCost;
		}

		for ( i = 2; i < pDTWSession->dtw.rows; i++ )
		{
			for ( j = 1; j < pDTWSession->dtw.cols; j++ )
			{
				if ( j < validCol )
				{
					currentCost = *( pDTWSession->frameCost.matrix.pDouble + ( (index + j) % pDTWSession->frameCost.matrix.rows ) * pDTWSession->frameCost.matrix.cols + i );

					arOptions[MATCH]  = *( pDTWSession->dtw.pDouble + (i - 1) * pDTWSession->dtw.cols + j - 1 ) + currentCost;
					arOptions[INSERT] = *( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j - 1 ) + currentCost;
					arOptions[DELETE] = *( pDTWSession->dtw.pDouble + (i - 2) * pDTWSession->dtw.cols + j - 1 ) + currentCost;

					*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j ) = AUD_MIN( arOptions[MATCH], AUD_MIN( arOptions[INSERT], arOptions[DELETE] ) );
				}
				else
				{
					*( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j ) = 500.0f;
				}
			}
		}
	}
	else
	{
		AUD_ASSERT( 0 );
	}

	AUD_Double warpTemplateLen;
	AUD_Int32s arGoalCell[2]  = { 0, 0 };
	AUD_Int32s arStartCell[2] = { 0, 0 };
	AUD_Double warpPathLen    = 0.;

	if ( scoringMethod == AUD_DTWSCORE_BEST )
	{
		/* compute goal cell */
		error = _computeGoalCell( &(pDTWSession->dtw), validCol, arGoalCell );
		if ( error == AUD_ERROR_MOREDATA )
		{
			*pScore = 500.;
			return AUD_ERROR_NONE;
		}
		// AUDLOG( "goal cell is %d x %d\n", arGoalCell[0], arGoalCell[1] );

		/* get the best path */
		error = _getPath( pDTWSession, arStartCell, arGoalCell, &warpPathLen, pPath );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		// AUDLOG( "start cell is %d x %d\n", arStartCell[0], arStartCell[1] );

		/* measure */
		warpTemplateLen = arGoalCell[0] - arStartCell[0];
		// int warpInputLen = arGoalCell[1] - arStartCell[1];

		// AUDLOG( "cell path is %d -> %d, len is %d\n", arStartCell[1], arGoalCell[1], warpInputLen );

		// AUDLOG( "template path len: %f\n", warpTemplateLen );
		*pScore = *( pDTWSession->dtw.pDouble + arGoalCell[0] * pDTWSession->dtw.cols + arGoalCell[1] ) / warpPathLen;
	}
	else if ( scoringMethod == AUD_DTWSCORE_TERMINAL )
	{
		warpTemplateLen = pDTWSession->dtw.rows;

		arGoalCell[0] = pDTWSession->dtw.rows - 1;
		arGoalCell[1] = pDTWSession->dtw.cols - 1;
		error = _getPath( pDTWSession, arStartCell, arGoalCell, &warpPathLen, pPath );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		*pScore = *( pDTWSession->dtw.pDouble + pDTWSession->dtw.rows * pDTWSession->dtw.cols - 1 ) / warpPathLen;
	}
	else
	{
		AUD_ASSERT( 0 );
	}

	if ( *pScore < 0. )
	{
		*pScore = 0.;
	}


#if DTW_DEBUG
	// print out frame cost matrix
	fprintf( fp, "frame cost:\n" );
	for ( i = 0; i < validCol; i++ )
	{
		for ( j = 0; j < pDTWSession->frameCost.matrix.cols; j++ )
		{
			fprintf( fp, "%3.2f, ", *(pDTWSession->frameCost.matrix.pDouble + i * pDTWSession->frameCost.matrix.cols + j) );
		}
		fprintf( fp, "\n" );
	}
	fprintf( fp, "\n\n" );

	// print out dtw matrix
	fprintf( fp, "dtw:\n" );
	for ( i = 0; i < pDTWSession->dtw.rows; i++ )
	{
		for ( j = 0; j < pDTWSession->dtw.cols; j++ )
		{
			fprintf( fp, "%3.2f, ", *( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + j ) );
		}
		fprintf( fp, "\n" );
	}
	fprintf( fp, "\n\n" );

	fprintf( fp, "(%s, %s, %d), score: %.2f\n", __FILE__, __FUNCTION__, __LINE__, *pScore );
#endif

	return AUD_ERROR_NONE;
}

static AUD_Error _getPath( AUD_DTWSession *pDTWSession, AUD_Int32s arStartCell[2], const AUD_Int32s arGoalCell[2], AUD_Double *pWarpPathLen, AUD_Vector *pPath )
{
	AUD_Int32s i = arGoalCell[0];
	AUD_Int32s j = arGoalCell[1];
	AUD_Int32s iStart = i, jStart = j;
	AUD_Double minVal;
	AUD_Double arOptions[3];
	AUD_Int32s option;
	AUD_Double warpPathLen = 0;

	if ( pPath )
	{
		*(pPath->pInt32s + i) = j;
	}

	while ( (i >= 1) && (j >= 1) )
	{
		arOptions[MATCH]  = *( pDTWSession->dtw.pDouble + ( i - 1 ) * pDTWSession->dtw.cols + ( j - 1 ) );
		arOptions[INSERT] = *( pDTWSession->dtw.pDouble + i * pDTWSession->dtw.cols + ( j - 1 ) );

		minVal = arOptions[MATCH];
		option = MATCH;

		if ( arOptions[INSERT] < arOptions[MATCH] )
		{
			minVal = arOptions[INSERT];
			option = INSERT;
		}

		if ( pDTWSession->transitionType == AUD_DTWTRANSITION_LEVENSHTEIN )
		{
			arOptions[DELETE] = *( pDTWSession->dtw.pDouble + ( i - 1 ) * pDTWSession->dtw.cols + j );
			if ( arOptions[DELETE] < minVal )
			{
				minVal = arOptions[DELETE];
				option = DELETE;
			}
		}
		else if ( pDTWSession->transitionType == AUD_DTWTRANSITION_DTW )
		{
			if ( i >= 2 )
			{
				arOptions[DELETE] = *( pDTWSession->dtw.pDouble + ( i - 2 ) * pDTWSession->dtw.cols + ( j - 1 ) );
				if ( arOptions[DELETE] < minVal )
				{
					minVal = arOptions[DELETE];
					option = DELETE;
				}
			}
		}

		if ( option == MATCH )
		{
			i = i - 1;
			j = j - 1;
			warpPathLen += sqrt( 2.0 );
		}
		else if ( option == INSERT )
		{
			j = j - 1;
			warpPathLen++;
		}
		else // if( option == DELETE )
		{
			if ( pDTWSession->transitionType == AUD_DTWTRANSITION_LEVENSHTEIN )
			{
				i = i - 1;
				warpPathLen++;
			}
			else if ( pDTWSession->transitionType == AUD_DTWTRANSITION_DTW )
			{
				i = i - 2;
				j = j - 1;
				warpPathLen += sqrt( 5.0 );
			}
		}

		if ( pPath )
		{
			*(pPath->pInt32s + i) = j;
		}

		iStart = i;
		jStart = j;
	}

	arStartCell[0] = iStart;
	arStartCell[1] = jStart;

	*pWarpPathLen = warpPathLen;

	return AUD_ERROR_NONE;
}

static AUD_Error _computeGoalCell( AUD_Matrix *pDTW, AUD_Int32s validCols, AUD_Int32s arGoalCell[2] )
{
	AUD_Int32s i = 0, j = 0;
	AUD_Int32s tolerancel = (AUD_Int32s)( pDTW->rows * WOV_DTWMATCH_FLOOR_RATIO );

	AUD_Double *pEndSlice = pDTW->pDouble + ( pDTW->rows - 1 ) * pDTW->cols;

	i = validCols - 1;
	if ( i < tolerancel )
	{
		return AUD_ERROR_MOREDATA;
	}

	for ( j = i - 1; j >= tolerancel; j-- )
	{
		if ( pEndSlice[j] < pEndSlice[i] )
		{
			i = j;
		}
	}

	arGoalCell[0] = pDTW->rows - 1;
	arGoalCell[1] = i;

	return AUD_ERROR_NONE;
}
