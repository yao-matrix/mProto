#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"

#include "math/mmath.h"

#include "benchmark.h"

// max allowable template number
#define TEMPLATE_MAX   5

AUD_Int32s test_dtw_mfcc()
{
	AUD_Int32s data;
	AUD_Int8s  inputPath[256]   = { 0, };
	AUD_Int8s  keywordPath[256] = { 0, };
	AUD_Double threshold        = 0.9;
	AUD_Error  error            = AUD_ERROR_NONE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give keyword wav stream's path:\n" );
	keywordPath[0] = '\0';
	data = scanf( "%s", keywordPath );
	AUDLOG( "keyword path is: %s\n", keywordPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give input wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "input path is: %s\n", inputPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give score threshold:\n" );
	data = scanf( "%lf", &threshold );
	AUDLOG( "threshold is: %.2f\n", threshold );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry              *pEntry       = NULL;
	dir                *pDir         = NULL;

	AUD_Int32s         i, j;
	AUD_Int32s         ret;

	AUD_DTWSession     arDtwSession[TEMPLATE_MAX];
	AUD_Feature        arKeyword[TEMPLATE_MAX];
	AUD_Int8s          arKeywordStream[TEMPLATE_MAX][256];
	AUD_Int8s          arKeywordName[TEMPLATE_MAX][256];
	AUD_Int32s         arKeywordSampleNum[TEMPLATE_MAX] = { 0, };
	AUD_Int32s         arKeywordWinNum[TEMPLATE_MAX]    = { 0, };
	AUD_Double         arScore[TEMPLATE_MAX]            = { 0., };
	AUD_Int32s         keywordNum = 0;

	AUD_Int32s         sampleNum;

	AUD_Int32s         bufLen  = 0;
	AUD_Int16s         *pBuf   = NULL;


	// train keyword
	bufLen   = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
	pBuf     = (AUD_Int16s*)calloc( bufLen, 1 );
	AUD_ASSERT( pBuf );


	AUDLOG( "***********************************************\n" );
	AUDLOG( "keyword training start...\n" );

	keywordNum = 0;

	pDir = openDir( (const char*)keywordPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		if ( keywordNum >= TEMPLATE_MAX )
		{
			AUDLOG( "out of max supported keyword[%d], will skip the following files\n", TEMPLATE_MAX );
			break;
		}

		snprintf( (char*)(arKeywordStream[keywordNum]), 256, "%s/%s", (const char*)keywordPath, pEntry->name );

		snprintf( (char*)(arKeywordName[keywordNum]), 256, "%s", pEntry->name );

		ret = readWavFromFile( arKeywordStream[keywordNum], pBuf, bufLen );
		if ( ret < 0 )
		{
			continue;
		}
		arKeywordSampleNum[keywordNum] = ret;

		// request memeory for template
		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= arKeywordSampleNum[keywordNum]; j++ )
		{
			;
		}
		AUDLOG( "pattern[%d: %s]: data size: %d, frame number: %d, frame len: %d, frame stride: %d\n",
		         keywordNum, arKeywordStream[keywordNum], arKeywordSampleNum[keywordNum], j, FRAME_LEN, FRAME_STRIDE );

		arKeywordWinNum[keywordNum]                  = j;
		arKeyword[keywordNum].featureMatrix.rows     = arKeywordWinNum[keywordNum] - MFCC_DELAY;
		arKeyword[keywordNum].featureMatrix.cols     = MFCC_FEATDIM;
		arKeyword[keywordNum].featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(arKeyword[keywordNum].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		arKeyword[keywordNum].featureNorm.len      = arKeywordWinNum[keywordNum] - MFCC_DELAY;
		arKeyword[keywordNum].featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(arKeyword[keywordNum].featureNorm) );
		AUD_ASSERT( ret == 0 );

		void *hMfccHandle = NULL;

		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, arKeywordSampleNum[keywordNum] );


		// calc MFCC feature
		error = mfcc16s32s_calc( hMfccHandle, pBuf, arKeywordSampleNum[keywordNum], &(arKeyword[keywordNum]) );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

#if 0
		char modelFile[256] = { 0, };
		strncpy( modelFile, (char*)(arKeywordStream[keywordNum]), 255 );
		strncpy( modelFile + strlen( modelFile ) - 3, "feat", 255 );

		ret = writeMatrixToFile( &(arDtwSession[keywordNum].feature.featureMatrix), (AUD_Int8s*)modelFile );
		AUD_ASSERT( ret == 0 );
#endif

		keywordNum++;
	}
	bufLen  = 0;
	free( pBuf );
	pBuf = NULL;
	closeDir( pDir );
	pEntry  = NULL;
	pDir    = NULL;


	AUDLOG( "total keyword number: %d\n", keywordNum );
	AUDLOG( "keyword training finish...\n" );
	AUDLOG( "***********************************************\n\n\n" );

	// read data from input file/stream and do matching
	pDir   = openDir( (const char*)inputPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Int8s   inputStream[256] = { 0, };
		void        *hMfccHandle     = NULL;
		AUD_Summary fileSummary;

		snprintf( (char*)inputStream, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( inputStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );
		AUDLOG( "input stream: %s\n", inputStream );

		bufLen  = fileSummary.dataChunkBytes;
		pBuf = (AUD_Int16s*)malloc( bufLen + 100 );
		AUD_ASSERT( pBuf );

		sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
		{
			;
		}

		AUD_Feature inFeature;
		inFeature.featureMatrix.rows     = j - MFCC_DELAY;
		inFeature.featureMatrix.cols     = MFCC_FEATDIM;
		inFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		inFeature.featureNorm.len      = j - MFCC_DELAY;
		inFeature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );


		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// dtw match
		for ( i = 0; i < keywordNum; i++ )
		{
			arDtwSession[i].dtwType        = WOV_DTW_TYPE;
			arDtwSession[i].distType       = WOV_DISTANCE_METRIC;
			arDtwSession[i].transitionType = WOV_DTWTRANSITION_STYLE;
			error = dtw_initsession( &(arDtwSession[i]), &(arKeyword[i]), inFeature.featureMatrix.rows );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_updatefrmcost( &(arDtwSession[i]), &inFeature );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_match( &(arDtwSession[i]), WOV_DTWSCORING_METHOD, &(arScore[i]), NULL );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_deinitsession( &(arDtwSession[i]) );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			if ( arScore[i] <= threshold )
			{
				AUDLOG( "\tFound pattern: %s, score: %.2f\n", arKeywordName[i], arScore[i] );
			}
		}

		free( pBuf );
		pBuf   = NULL;
		bufLen = 0;

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		AUDLOG( "\n\n" );
	}
	closeDir( pDir );
	pDir = NULL;

	for ( i = 0; i < keywordNum; i++ )
	{
		ret = destroyMatrix( &(arKeyword[i].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(arKeyword[i].featureNorm) );
		AUD_ASSERT( ret == 0 );
	}


	AUDLOG( "DTW-MFCC test finished\n" );

	return 0;
}

