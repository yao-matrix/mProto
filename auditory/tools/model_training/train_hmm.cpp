/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "io/file/io.h"

AUD_Int32s train_keyword_hmm( const AUD_Int8s *pKeywordFile, AUD_Int8s *pHmmName )
{
	AUD_Error  error = AUD_ERROR_NONE;

	// step 1: read garbage model from file
	void *hGarbageGmm = NULL;
	FILE *fpGarbage = fopen( (char*)WOV_UBM_GMMHMMMODEL_FILE, "rb" );
	if ( fpGarbage == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", WOV_UBM_GMMHMMMODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}

	error = gmm_import( &hGarbageGmm, fpGarbage );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	fclose( fpGarbage );
	fpGarbage = NULL;

	// AUDLOG( "garbage GMM as:\n" );
	// gmm_show( hGarbageGmm );

	// step 2: read template stream & extract MFCC feature vector
	AUD_Int32s sampleNum = 0;
	AUD_Int32s bufLen    = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
	AUD_Int16s *pBuf     = (AUD_Int16s*)calloc( bufLen, 1 );
	AUD_ASSERT( pBuf );

	AUD_Int32s ret;

	// read stream from file
	sampleNum = readWavFromFile( (AUD_Int8s*)pKeywordFile, pBuf, bufLen );
	AUD_ASSERT( sampleNum > 0 );

	AUD_Int32s i = 0, j = 0, k = 0, m = 0;

	// front end processing

	 // pre-emphasis
	sig_preemphasis( pBuf, pBuf, sampleNum );


	// calc frame number
	for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
	{
		;
	}

	AUD_Feature feature;
	feature.featureMatrix.rows     = j - MFCC_DELAY;
	feature.featureMatrix.cols     = MFCC_FEATDIM;
	feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &(feature.featureMatrix) );
	AUD_ASSERT( ret == 0 );

	feature.featureNorm.len      = j - MFCC_DELAY;
	feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
	ret = createVector( &(feature.featureNorm) );
	AUD_ASSERT( ret == 0 );

	// init mfcc handle
	void *hMfccHandle = NULL;
	error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	// calc MFCC feature
	error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	free( pBuf );
	pBuf = NULL;

	// step 3: for each feature vector, get the bestN most likelihood component indices from GMM
	AUD_Vector componentLLR;
	componentLLR.len      = gmm_getmixnum( hGarbageGmm );
	componentLLR.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &componentLLR );
	AUD_ASSERT( ret == 0 );

	AUD_Matrix indexTable;
	indexTable.rows     = feature.featureMatrix.rows ;
	indexTable.cols     = WOV_KEYWORD_GMMMODEL_ORDER;
	indexTable.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &indexTable );
	AUD_ASSERT( ret == 0 );

	AUD_Matrix llrTable;
	llrTable.rows     = feature.featureMatrix.rows;
	llrTable.cols     = WOV_KEYWORD_GMMMODEL_ORDER;
	llrTable.dataType = AUD_DATATYPE_DOUBLE;
	ret = createMatrix( &llrTable );
	AUD_ASSERT( ret == 0 );

	AUD_Double totalLLR;

	for ( i = 0; i < feature.featureMatrix.rows; i++ )
	{
		totalLLR = gmm_llr( hGarbageGmm, &(feature.featureMatrix), i, &componentLLR );

#if 0
		showVector( &componentLLR );
#endif

		// sort the bestN likelihood
		AUD_Int32s *pIndex = indexTable.pInt32s + i * indexTable.cols;
		AUD_Double *pLLR   = llrTable.pDouble + i * llrTable.cols;

		for ( j = 0; j < WOV_KEYWORD_GMMMODEL_ORDER; j++ )
		{
			pIndex[j] = -1;
			pLLR[j]   = 0.;
		}

		for ( j = 0; j < componentLLR.len; j++ )
		{
			for ( k = 0; k < WOV_KEYWORD_GMMMODEL_ORDER; k++ )
			{
				if ( pIndex[k] == -1 )
				{
					pIndex[k] = j;
					pLLR[k]   = componentLLR.pDouble[j];
					break;
				}
				else if ( componentLLR.pDouble[j] > pLLR[k] )
				{
					for ( m = WOV_KEYWORD_GMMMODEL_ORDER - 1; m > k ; m-- )
					{
						pIndex[m] = pIndex[m - 1];
						pLLR[m]   = pLLR[m - 1];
					}
					pIndex[k] = j;
					pLLR[k]   = componentLLR.pDouble[j];
					break;
				}
			}
		}
	}

#if 0
	AUDLOG( "index table( %s, %s, %d ):\n", __FILE__, __FUNCTION__, __LINE__ );
	showMatrix( &indexTable );
	AUDLOG( "llr table( %s, %s, %d ):\n", __FILE__, __FUNCTION__, __LINE__ );
	showMatrix( &llrTable );
#endif

	ret = destroyVector( &componentLLR );
	AUD_ASSERT( ret == 0 );

	// step 4: cluster GMM
	AUD_Int32s *pClusterLabel = (AUD_Int32s*)calloc( sizeof(AUD_Int32s) * feature.featureMatrix.rows, 1 );
	AUD_ASSERT( pClusterLabel );

	error = gmm_cluster( hGarbageGmm, &indexTable, WOV_GMM_CLUSTER_THRESHOLD, pClusterLabel );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUD_Int32s stateNum = pClusterLabel[feature.featureMatrix.rows - 1];
	AUD_ASSERT( stateNum >= 5 );

	// step 5: select and build state GMM
	void **phKeywordGmms = (void**)calloc( sizeof(void*) * stateNum, 1 );
	AUD_ASSERT( phKeywordGmms );

	AUD_Vector indexVector;
	indexVector.len      = WOV_KEYWORD_GMMMODEL_ORDER;
	indexVector.dataType = AUD_DATATYPE_INT32S;
	ret = createVector( &indexVector );
	AUD_ASSERT( ret == 0 );

	AUD_Vector llrVector;
	llrVector.len      = WOV_KEYWORD_GMMMODEL_ORDER;
	llrVector.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &llrVector );
	AUD_ASSERT( ret == 0 );

	int start = 0, end = 0;
	for ( i = 0; i < stateNum; i++ )
	{
		for ( j = 0; j < indexVector.len; j++ )
		{
			indexVector.pInt32s[j] = -1;
			llrVector.pInt32s[j]   = 1.;
		}

		for ( j = start; j < feature.featureMatrix.rows; j++ )
		{
			if ( pClusterLabel[j] != i )
			{
				break;
			}
		}
		end = j;

		for ( k = start * llrTable.cols; k < end * llrTable.cols; k++ )
		{
			for ( m = 0; m < indexVector.len; m++ )
			{
				if ( llrTable.pDouble[k] == llrVector.pDouble[m] &&
				     indexTable.pInt32s[k] == indexVector.pInt32s[m] )
				{
					break;
				}
				else if ( indexVector.pInt32s[m] == -1 || llrTable.pDouble[k] > llrVector.pDouble[m] )
				{
					for ( int n = indexVector.len - 1; n > m ; n-- )
					{
						indexVector.pInt32s[n] = indexVector.pInt32s[n - 1];
						llrVector.pDouble[n]   = llrVector.pDouble[n - 1];
					}
					indexVector.pInt32s[m] = indexTable.pInt32s[k];
					llrVector.pDouble[m]   = llrTable.pDouble[k];
					break;
				}
			}
		}

		// AUDLOG( "Final GMM indices for state[%d]:\n", i );
		// showVector( &indexVector );

		AUD_Int8s gmmName[256] = { 0, };
		sprintf( (char*)gmmName, "state%d", i );
		error = gmm_select( &phKeywordGmms[i], hGarbageGmm, &indexVector, 0, gmmName );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		start = end;
	}

	ret = destroyMatrix( &indexTable );
	AUD_ASSERT( ret == 0 );

	ret = destroyMatrix( &llrTable );
	AUD_ASSERT( ret == 0 );

	ret = destroyVector( &indexVector );
	AUD_ASSERT( ret == 0 );

	ret = destroyVector( &llrVector );
	AUD_ASSERT( ret == 0 );

	free( pClusterLabel );
	pClusterLabel = NULL;

	// step 6: generate keyword model by Baum-Welch algorithm
	AUD_Vector pi;
	pi.len      = stateNum;
	pi.dataType = AUD_DATATYPE_DOUBLE;
	ret = createVector( &pi );
	AUD_ASSERT( ret == 0 );

	pi.pDouble[0] = 1.0f;
	void *hKeywordHmm   = NULL;
	error = gmmhmm_init( &hKeywordHmm, stateNum, &pi, phKeywordGmms );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmmhmm_learn( hKeywordHmm, &feature, 1, 0.001 );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	// step 8: write model to file
	error = gmmhmm_export( hKeywordHmm, pHmmName );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	// gmmhmm_show( hKeywordHmm );

	// clean field
	error = mfcc16s32s_deinit( &hMfccHandle );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmm_free( &hGarbageGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	ret = destroyMatrix( &(feature.featureMatrix) );
	AUD_ASSERT( ret == 0 );

	ret = destroyVector( &(feature.featureNorm) );
	AUD_ASSERT( ret == 0 );

	ret = destroyVector( &pi );
	AUD_ASSERT( ret == 0 );


	for ( i = 0; i < stateNum; i++ )
	{
		error = gmm_free( &phKeywordGmms[i] );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}
	free( phKeywordGmms );
	phKeywordGmms = NULL;

	error = gmmhmm_free( &hKeywordHmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	return 0;
}
 
int train_hmm()
{
	AUD_Int8s  filePath[256] = { 0, };
	AUD_Int32s data;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give training wav stream's folder path:\n" );
	filePath[0] = '\0';
	data = scanf( "%s", filePath );
	AUDLOG( "training folder path is: %s\n", filePath );

	entry *pEntry = NULL;
	dir   *pDir   = NULL;

	pDir = openDir( (const char*)filePath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		// skip non wav file
		char suffix[256] = { 0, };
		for ( int i = 0; i < (int)strlen( pEntry->name ); i++ )
		{
			suffix[i] = tolower( pEntry->name[i] );
		}
		suffix[strlen( pEntry->name )] = '\0';

		if ( strstr( suffix, ".wav" ) == NULL )
		{
			continue;
		}

		char fileName[256] = { 0, };
		sprintf( fileName, "%s/%s", (const char*)filePath, pEntry->name );

		char hmmName[256] = { 0, };
		sprintf( hmmName, "%s/%s", WOV_KEYWORD_GMMHMMMODEL_DIR, pEntry->name );
		strcpy( hmmName + strlen( hmmName ) - 3, "hmm" );

		train_keyword_hmm( (const AUD_Int8s*)fileName, (AUD_Int8s*)hmmName );
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "HMM training done\n" );

	return 0;
}

