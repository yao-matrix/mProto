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

AUD_Int32s perf_dtw_mfcc_vad()
{
	char            inputPath[256]   = { 0, };
	char            keywordPath[256] = { 0, };
	AUD_Double      threshold, minThreshold, maxThreshold, stepThreshold;
	AUD_Bool        isRecognized     = AUD_FALSE;
	AUD_Int32s      data;

	AUD_Error       error = AUD_ERROR_NONE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give template wav stream's path:\n" );
	keywordPath[0] = '\0';
	data = scanf( "%s", keywordPath );
	AUDLOG( "template path is: %s\n", keywordPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give lowest test threshold:\n" );
	data = scanf( "%lf", &minThreshold );
	AUDLOG( "lowest threshold is: %.2f\n", minThreshold );

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

	AUD_Int32s      i, j;

	AUD_Feature     keywordFeature;
	AUD_Int32s      keywordSampleNum = 0;
	AUD_Int32s      keywordWinNum    = 0;
	AUD_Int8s       keywordFile[256] = { 0, };
	AUD_Int8s       keywordName[256] = { 0, };
	AUD_Int32s      keywordID        = 0;

	AUD_DTWSession  dtwSession;
	AUD_Double      score             = 0.;
	AUD_Int32s      sampleNum;

	AUD_Int32s      keywordBufLen = 0;
	AUD_Int16s      *pKeywordBuf  = NULL;
	AUD_Int32s      bufLen        = 0;
	AUD_Int16s      *pBuf         = NULL;

	AUD_Int32s      ret;

	// init performance benchmark
	void *hBenchmark  = NULL;
	char logName[256] = { 0, };
	snprintf( logName, 256, "dtw-mfcc-vad-%d-%d-%d-%d", WOV_DTW_TYPE, WOV_DISTANCE_METRIC, WOV_DTWTRANSITION_STYLE, WOV_DTWSCORING_METHOD );
	ret = benchmark_init( &hBenchmark, (AUD_Int8s*)logName, keywords, sizeof(keywords) / sizeof(keywords[0]) );
	AUD_ASSERT( ret == 0 );

	for ( threshold = minThreshold; threshold < maxThreshold + stepThreshold; threshold += stepThreshold )
	{
		int frameStride = 0;

		ret = benchmark_newthreshold( hBenchmark, threshold );
		AUD_ASSERT( ret == 0 );

		AUDLOG( "***********************************************\n" );
		AUDLOG( "keyword training start...\n" );

		pKeywordDir = openDir( (const char*)keywordPath );
		keywordID  = 0;
		while ( ( pKeywordEntry = scanDir( pKeywordDir ) ) )
		{
			// train keyword
			AUD_Summary fileSummary;

			snprintf( (char*)keywordFile, 256, "%s/%s", (const char*)keywordPath, pKeywordEntry->name );

			ret = parseWavFromFile( keywordFile, &fileSummary );
			if ( ret < 0 )
			{
				continue;
			}
			AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

			keywordBufLen = fileSummary.dataChunkBytes + 100;
			pKeywordBuf   = (AUD_Int16s*)calloc( keywordBufLen, 1 );
			AUD_ASSERT( pKeywordBuf );

			keywordSampleNum = readWavFromFile( keywordFile, pKeywordBuf, keywordBufLen );
			if ( keywordSampleNum < 0 )
			{
				continue;
			}

			// front end processing
			 // pre-emphasis
			sig_preemphasis( pKeywordBuf, pKeywordBuf, keywordSampleNum );


			for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= keywordSampleNum; j++ )
			{
				;
			}
			frameStride = FRAME_STRIDE;

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

			void *hTemplateMfccHandle = NULL;

			// init mfcc handle
			error = mfcc16s32s_init( &hTemplateMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// calc MFCC feature
			error = mfcc16s32s_calc( hTemplateMfccHandle, pKeywordBuf, keywordSampleNum, &keywordFeature );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// deinit mfcc handle
			error = mfcc16s32s_deinit( &hTemplateMfccHandle );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			strncpy( (char*)keywordName, pKeywordEntry->name, 255 );
			ret = benchmark_newtemplate( hBenchmark, keywordName );
			AUD_ASSERT( ret == 0 );

			keywordID++;

			keywordBufLen  = 0;
			free( pKeywordBuf );
			pKeywordBuf = NULL;

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

				bufLen  = fileSummary.dataChunkBytes;
				pBuf = (AUD_Int16s*)malloc( bufLen + 100 );
				AUD_ASSERT( pBuf );

				sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );

				// VAD
				AUD_Int16s *pValidBuf = (AUD_Int16s*)calloc( bufLen, 1 );
				AUD_ASSERT( pValidBuf );
				AUD_Int16s *pSpeechFlag  = (AUD_Int16s*)calloc( ( sampleNum / VAD_FRAME_LEN ) + 1, sizeof( AUD_Int16s ) );
				AUD_ASSERT( pSpeechFlag );

				void *hVadHandle = NULL;
				ret = sig_vadinit( &hVadHandle, pValidBuf, pSpeechFlag, bufLen, -1, -1 );
				AUD_ASSERT( ret == 0 );

				AUD_Int32s start = -1;
				for ( i = 0; i * VAD_FRAME_LEN + FRAME_LEN <= sampleNum; i++ )
				{
					ret = sig_vadupdate( hVadHandle, pBuf + i * VAD_FRAME_LEN, &start );
					if ( ret == 0 )
					{
						continue;
					}
					else
					{
						break;
					}
				}

				if ( ret == 0 )
				{
					continue;
				}

				AUD_Int32s validSampleNum = ret;
				sig_vaddeinit( &hVadHandle );

#if 1
				char vadFile[256] = { 0, };
				snprintf( vadFile, 255, "./vaded/%s", pEntry->name );
				ret = writeWavToFile( (AUD_Int8s*)vadFile, pValidBuf + start, validSampleNum );
				AUD_ASSERT( ret == 0 );
#endif

				// de-noise
				AUD_Int16s *pCleanBuf = (AUD_Int16s*)calloc( (validSampleNum + start) * BYTES_PER_SAMPLE, 1 );
				AUD_ASSERT( pCleanBuf );

				AUD_Int32s cleanLen = denoise_mmse( pValidBuf, pCleanBuf, (validSampleNum + start) );

#if 1
				char cleanFile[256] = { 0, };
				snprintf( cleanFile, 255, "./clean/%s", pEntry->name );
				ret = writeWavToFile( (AUD_Int8s*)cleanFile, pCleanBuf, cleanLen );
				AUD_ASSERT( ret == 0 );
#endif
				// front end processing
				 // pre-emphasis
				sig_preemphasis( pCleanBuf, pCleanBuf, cleanLen );

				for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= cleanLen; j++ )
				{
					;
				}
				frameStride = FRAME_LEN;

				bufLen = 0;
				free( pBuf );
				pBuf = NULL;
				free( pValidBuf );
				pValidBuf = NULL;
				free( pSpeechFlag );
				pSpeechFlag = NULL;

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

				error = mfcc16s32s_calc( hMfccHandle, pCleanBuf, cleanLen, &inFeature );
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

				AUDLOG( "score: %.2f\n", score );

				isRecognized = AUD_FALSE;
				if ( score <= threshold )
				{
					isRecognized = AUD_TRUE;
				}

				// insert log entry
				ret = benchmark_additem( hBenchmark, (AUD_Int8s*)pEntry->name, score, isRecognized );
				AUD_ASSERT( ret == 0 );

				cleanLen = 0;
				free( pCleanBuf );
				pCleanBuf = NULL;
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

	AUDLOG( "DTW-MFCC VAD Benchmark finished\n" );

	return 0;
}
