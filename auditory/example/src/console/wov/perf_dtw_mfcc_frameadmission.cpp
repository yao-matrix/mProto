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

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"
#include "math/mmath.h"

#include "benchmark.h"


static const char* keywords[] =
{
	"academy",
	"computer",
	"geniusbutton",
	"hamburger",
	"hellobluegenie",
	"hellointel",
	"heyvlingo",
	"listentome",
	"original",
	"startlistening",
	"wakeup",
};


AUD_Int32s perf_dtw_mfcc_frameadmit()
{
	char            inputPath[256]   = { 0, };
	char            keywordPath[256] = { 0, };
	AUD_Double      threshold, minThreshold, maxThreshold, stepThreshold;
	AUD_Bool        isRecognized     = AUD_FALSE;
	AUD_Int32s      data;

	AUD_Error       error = AUD_ERROR_NONE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give keyword wav stream's path:\n" );
	keywordPath[0] = '\0';
	data = scanf( "%s", keywordPath );
	AUDLOG( "keyword path is: %s\n", keywordPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give min test threshold:\n" );
	data = scanf( "%lf", &minThreshold );
	AUDLOG( "min threshold is: %.2f\n", minThreshold );

	setbuf( stdin, NULL );
	AUDLOG( "pls give max test threshold:\n" );
	data = scanf( "%lf", &maxThreshold );
	AUDLOG( "max threshold is: %.2f\n", maxThreshold );

	setbuf( stdin, NULL );
	AUDLOG( "pls give test threshold step:\n" );
	data = scanf( "%lf", &stepThreshold );
	AUDLOG( "threshold step is: %.2f\n", stepThreshold );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry   *pEntry        = NULL;
	dir     *pDir          = NULL;
	entry   *pKeywordEntry = NULL;
	dir     *pKeywordDir   = NULL;

	AUD_Feature     keywordFeature;
	AUD_Int32s      keywordSampleNum = 0;
	AUD_Int32s      keywordWinNum    = 0;
	AUD_Int8s       keywordFile[256] = { 0, };
	AUD_Int8s       keywordName[256] = { 0, };
	AUD_Int32s      keywordID        = 0;

	AUD_DTWSession  dtwSession;
	AUD_Double      score            = 0.;
	AUD_Int32s      sampleNum;

	AUD_Int32s      keywordBufLen = 0;
	AUD_Int16s      *pKeywordBuf  = NULL;
	AUD_Int32s      bufLen        = 0;
	AUD_Int16s      *pBuf         = NULL;

	AUD_Int32s      frameStride   = FRAME_STRIDE;

	AUD_Int32s      j;
	AUD_Int32s      ret;

	// init performance benchmark
	void *hBenchmark  = NULL;
	char logName[256] = { 0, };
	snprintf( logName, 256, "dtw-mfcc-frameadmit-%d-%d-%d-%d", WOV_DTW_TYPE, WOV_DISTANCE_METRIC, WOV_DTWTRANSITION_STYLE, WOV_DTWSCORING_METHOD );
	ret = benchmark_init( &hBenchmark, (AUD_Int8s*)logName, keywords, sizeof(keywords) / sizeof(keywords[0]) );
	AUD_ASSERT( ret == 0 );


	for ( threshold = minThreshold; threshold < maxThreshold + stepThreshold; threshold += stepThreshold )
	{
		ret = benchmark_newthreshold( hBenchmark, threshold );
		AUD_ASSERT( ret == 0 );

		AUDLOG( "***********************************************\n" );
		AUDLOG( "keyword training start...\n" );

		pKeywordDir = openDir( (const char*)keywordPath );
		keywordID  = 0;
		while ( ( pKeywordEntry = scanDir( pKeywordDir ) ) )
		{
			// train keyword
			keywordBufLen  = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
			pKeywordBuf = (AUD_Int16s*)calloc( keywordBufLen, 1 );
			AUD_ASSERT( pKeywordBuf );

			snprintf( (char*)keywordFile, 256, "%s/%s", (const char*)keywordPath, pKeywordEntry->name );

			keywordSampleNum = readWavFromFile( keywordFile, pKeywordBuf, keywordBufLen );
			if ( keywordSampleNum < 0 )
			{
				continue;
			}

			// front end processing
			 // pre-emphasis
			sig_preemphasis( pKeywordBuf, pKeywordBuf, keywordSampleNum );


			 // frame admission
			AUD_Int32s validKeywordBufLen = keywordSampleNum / FRAME_STRIDE * FRAME_LEN * BYTES_PER_SAMPLE;
			AUD_Int16s *pValidKeywordBuf  = (AUD_Int16s*)calloc( validKeywordBufLen, 1 );
			AUD_ASSERT( pValidKeywordBuf );

			AUD_Int32s validKeywordSampleNum = 0;
			validKeywordSampleNum = sig_frameadmit( pKeywordBuf, keywordSampleNum, pValidKeywordBuf );

			j           = validKeywordSampleNum / FRAME_LEN;
			frameStride = FRAME_LEN;

			AUDLOG( "pattern[%d: %s] valid frame number[%d]\n", keywordID, keywordFile, j );

			keywordWinNum                         = j;
			keywordFeature.featureMatrix.rows     = keywordWinNum - MFCC_DELAY;
			keywordFeature.featureMatrix.cols     = MFCC_FEATDIM;
			keywordFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
			ret = createMatrix( &(keywordFeature.featureMatrix) );
			AUD_ASSERT( ret == 0 );

			keywordFeature.featureNorm.len      = keywordWinNum - MFCC_DELAY;
			keywordFeature.featureNorm.dataType = AUD_DATATYPE_INT64S;
			ret = createVector( &(keywordFeature.featureNorm) );
			AUD_ASSERT( ret == 0 );

			void *hKeywordMfccHandle = NULL;

			// init mfcc handle
			error = mfcc16s32s_init( &hKeywordMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// calc MFCC feature
			error = mfcc16s32s_calc( hKeywordMfccHandle, pValidKeywordBuf, validKeywordSampleNum, &keywordFeature );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// deinit mfcc handle
			error = mfcc16s32s_deinit( &hKeywordMfccHandle );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			strncpy( (char*)keywordName, pKeywordEntry->name, 255 );
			ret = benchmark_newtemplate( hBenchmark, keywordName );
			AUD_ASSERT( ret == 0 );

			keywordID++;

			keywordBufLen         = 0;
			free( pKeywordBuf );
			pKeywordBuf           = NULL;
			validKeywordSampleNum = 0;
			free( pValidKeywordBuf );
			pValidKeywordBuf      = NULL;

			AUDLOG( "keyword training finish...\n" );

			// recognize
			pEntry = NULL;
			pDir   = NULL;
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

				bufLen = fileSummary.dataChunkBytes;
				pBuf   = (AUD_Int16s*)calloc( bufLen + 100, 1 );
				AUD_ASSERT( pBuf );

				sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );


				// front end processing
				 // pre-emphasis
				sig_preemphasis( pBuf, pBuf, sampleNum );


				 // frame admission
				AUD_Int32s validBufLen = sampleNum / FRAME_STRIDE * FRAME_LEN * BYTES_PER_SAMPLE;
				AUD_Int16s *pValidBuf  = (AUD_Int16s*)calloc( validBufLen, 1 );
				AUD_ASSERT( pValidBuf );

				AUD_Int32s validSampleNum = 0;
				validSampleNum = sig_frameadmit( pBuf, sampleNum, pValidBuf );

				j           = validSampleNum / FRAME_LEN;
				frameStride = FRAME_LEN;

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


				// init mfcc handle
				error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = mfcc16s32s_calc( hMfccHandle, pValidBuf, validSampleNum, &inFeature );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// init dtw match
				dtwSession.dtwType        = WOV_DTW_TYPE;
				dtwSession.distType       = WOV_DISTANCE_METRIC;
				dtwSession.transitionType = WOV_DTWTRANSITION_STYLE;
				error = dtw_initsession( &dtwSession, &keywordFeature, inFeature.featureMatrix.rows );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = dtw_updatefrmcost( &dtwSession, &inFeature );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = dtw_match( &dtwSession, WOV_DTWSCORING_METHOD, &score, NULL );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = dtw_deinitsession( &dtwSession );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// deinit mfcc handle
				error = mfcc16s32s_deinit( &hMfccHandle );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				ret = destroyMatrix( &(inFeature.featureMatrix) );
				AUD_ASSERT( ret == 0 );

				ret = destroyVector( &(inFeature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				isRecognized = AUD_FALSE;
				if ( score <= threshold )
				{
					isRecognized = AUD_TRUE;
				}

				AUDLOG( "score: %.2f\n", score );

				// insert log entry
				ret = benchmark_additem( hBenchmark, (AUD_Int8s*)pEntry->name, score, isRecognized );
				AUD_ASSERT( ret == 0 );

				bufLen      = 0;
				free( pBuf );
				pBuf        = NULL;
				validBufLen = 0;
				free( pValidBuf );
				pValidBuf   = NULL;
			}
			closeDir( pDir );
			pDir = NULL;

			ret = benchmark_finalizetemplate( hBenchmark );
			AUD_ASSERT( ret == 0 );

			ret = destroyMatrix( &(keywordFeature.featureMatrix) );
			AUD_ASSERT( ret == 0 );

			ret = destroyVector( &(keywordFeature.featureNorm) );
			AUD_ASSERT( ret == 0 );
		}
		closeDir( pKeywordDir );
		pKeywordDir = NULL;

		ret = benchmark_finalizethreshold( hBenchmark );
		AUD_ASSERT( ret == 0 );
	}

	ret = benchmark_free( &hBenchmark );
	AUD_ASSERT( ret == 0 );

	AUDLOG( "DTW-MFCC Benchmark finished\n" );

	return 0;
}
