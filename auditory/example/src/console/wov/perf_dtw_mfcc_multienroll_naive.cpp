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

static int keywordNum = sizeof(keywords) / sizeof(keywords[0]);
static int speakerNum = 11;

#define ENROLLMENT_NUM  3

AUD_Int32s perf_dtw_mfcc_multienroll_naive()
{
	const char      *pInputPath   = "./test";
	const char      *pKeywordPath = "./keyword_sk";
	AUD_Double      threshold, minThreshold, maxThreshold, stepThreshold;
	AUD_Bool        isRecognized  = AUD_FALSE;
	AUD_Int32s      data;

	AUD_Error       error = AUD_ERROR_NONE;

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
	entry          *pEntry         = NULL;
	dir            *pDir           = NULL;

	AUD_Int32s      i, j;

	AUD_DTWSession  arDtwSession[ENROLLMENT_NUM];
	AUD_Feature     arKeyword[ENROLLMENT_NUM];
	AUD_Int32s      arKeywordSampleNum[ENROLLMENT_NUM] = { 0, };
	AUD_Int32s      arKeywordWinNum[ENROLLMENT_NUM]    = { 0, };
	AUD_Int32s      keywordID                          = 0;

	AUD_Int8s       keywordFile[256];
	AUD_Int8s       keywordName[256];

	AUD_Double      arScore[ENROLLMENT_NUM]            = { 0., };
	AUD_Int32s      vote;
	AUD_Bool        pass;

	AUD_Int32s      sampleNum;
	AUD_Int32s      keywordBufLen = 0;
	AUD_Int16s      *pKeywordBuf  = NULL;
	AUD_Int32s      bufLen        = 0;
	AUD_Int16s      *pBuf         = NULL;

	AUD_Int32s      ret;
	AUD_Int32s      speakerID;

	keywordBufLen  = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
	pKeywordBuf = (AUD_Int16s*)calloc( keywordBufLen, 1 );
	AUD_ASSERT( pKeywordBuf != NULL );

	// init performance benchmark
	void *hBenchmark  = NULL;
	char logName[256] = { 0, };
	snprintf( logName, 256, "dtw-mfcc-multienroll-naive-%d-%d-%d-%d", WOV_DTW_TYPE, WOV_DISTANCE_METRIC, WOV_DTWTRANSITION_STYLE, WOV_DTWSCORING_METHOD );
	ret = benchmark_init( &hBenchmark, (AUD_Int8s*)logName, keywords, keywordNum );
	AUD_ASSERT( ret == 0 );

	for ( threshold = minThreshold; threshold < maxThreshold + stepThreshold; threshold += stepThreshold )
	{
		ret = benchmark_newthreshold( hBenchmark, threshold );
		AUD_ASSERT( ret == 0 );

		for ( keywordID = 0; keywordID < keywordNum; keywordID++ )
		{
			// train template
			for ( speakerID = 1; speakerID <= speakerNum; speakerID++ )
			{
				AUDLOG( "***********************************************\n" );
				AUDLOG( "template training start...\n" );
				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					snprintf( (char*)keywordFile, 256, "%s/%s/%d-%s-%d_16k.wav", pKeywordPath, keywords[keywordID], speakerID, keywords[keywordID], i + 1 );
					AUDLOG( "%s\n", keywordFile );
					arKeywordSampleNum[i] = readWavFromFile( keywordFile, pKeywordBuf, keywordBufLen );
					AUD_ASSERT( arKeywordSampleNum[i] > 0 );

					// request memeory for template
					for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= arKeywordSampleNum[i]; j++ )
					{
						;
					}

					arKeywordWinNum[i]                  = j;
					arKeyword[i].featureMatrix.rows     = arKeywordWinNum[i] - MFCC_DELAY;
					arKeyword[i].featureMatrix.cols     = MFCC_FEATDIM;
					arKeyword[i].featureMatrix.dataType = AUD_DATATYPE_INT32S;
					ret = createMatrix( &(arKeyword[i].featureMatrix) );
					AUD_ASSERT( ret == 0 );

					arKeyword[i].featureNorm.len      = arKeywordWinNum[i] - MFCC_DELAY;
					arKeyword[i].featureNorm.dataType = AUD_DATATYPE_INT64S;
					ret = createVector( &(arKeyword[i].featureNorm) );
					AUD_ASSERT( ret == 0 );

					void *hKeywordMfccHandle = NULL;

					// front end processing
					 // pre-emphasis
					sig_preemphasis( pKeywordBuf, pKeywordBuf, arKeywordSampleNum[i] );


					// init mfcc handle
					error = mfcc16s32s_init( &hKeywordMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// calc MFCC feature
					error = mfcc16s32s_calc( hKeywordMfccHandle, pKeywordBuf, arKeywordSampleNum[i], &(arKeyword[i]) );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// deinit mfcc handle
					error = mfcc16s32s_deinit( &hKeywordMfccHandle );
					AUD_ASSERT( error == AUD_ERROR_NONE );
				}

				snprintf( (char*)keywordName, 256, "%d-%s-", speakerID, keywords[keywordID] );

				ret = benchmark_newtemplate( hBenchmark, keywordName );
				AUD_ASSERT( ret == 0 );

				AUDLOG( "keyword[%d: %s] training finish...\n", keywordID, keywordName );

				// recognize
				pEntry = NULL;
				pDir   = NULL;
				pDir   = openDir( (const char*)pInputPath );
				while ( ( pEntry = scanDir( pDir ) ) )
				{
					AUD_Int8s   inputStream[256] = { 0, };
					void        *hMfccHandle     = NULL;
					AUD_Summary fileSummary;

					snprintf( (char*)inputStream, 256, "%s/%s", (const char*)pInputPath, pEntry->name );

					ret = parseWavFromFile( inputStream, &fileSummary );
					if ( ret < 0 )
					{
						continue;
					}
					AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );
					// AUDLOG( "input stream: %s\n", inputStream );

					bufLen  = fileSummary.dataChunkBytes;
					pBuf = (AUD_Int16s*)calloc( bufLen + 100, 1 );
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

					vote = 0;
					pass = AUD_FALSE;

					// dtw match
					for ( i = 0; i < ENROLLMENT_NUM; i++ )
					{
						arDtwSession[i].dtwType        = WOV_DTW_TYPE;
						arDtwSession[i].distType       = WOV_DISTANCE_METRIC;
						arDtwSession[i].transitionType = WOV_DTWTRANSITION_STYLE;
						error = dtw_initsession( &(arDtwSession[i]), &(arKeyword[i]), inFeature.featureMatrix.rows );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						// update frame cost
						error = dtw_updatefrmcost( &(arDtwSession[i]), &inFeature );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						// compute DTW
						error = dtw_match( &(arDtwSession[i]), WOV_DTWSCORING_METHOD, &(arScore[i]), NULL );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						if ( arScore[i] < threshold )
						{
							vote++;
						}

						if ( arScore[i] < 0.1 )
						{
							pass = AUD_TRUE;
						}

						error = dtw_deinitsession( &(arDtwSession[i]) );
						AUD_ASSERT( error == AUD_ERROR_NONE );
					}

					if ( vote > ENROLLMENT_NUM / 2 || pass == AUD_TRUE )
					{
						isRecognized = AUD_TRUE;
						AUDLOG( "\t!!! [%-30s] MATCH pattern[%-20s], score: [%d: %.2f, %.2f, %.2f]\n", pEntry->name, keywordName, vote, arScore[0], arScore[1], arScore[2] );
					}
					else
					{
						isRecognized = AUD_FALSE;
					}

					free( pBuf );
					bufLen = 0;
					pBuf   = NULL;
					
					// deinit mfcc handle
					error = mfcc16s32s_deinit( &hMfccHandle );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					ret = destroyMatrix( &(inFeature.featureMatrix) );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &(inFeature.featureNorm) );
					AUD_ASSERT( ret == 0 );

					// insert log entry
					ret = benchmark_additem( hBenchmark, (AUD_Int8s*)pEntry->name, (AUD_Double)vote, isRecognized );
					AUD_ASSERT( ret == 0 );
				}
				closeDir( pDir );
				pDir = NULL;

				ret = benchmark_finalizetemplate( hBenchmark );
				AUD_ASSERT( ret == 0 );

				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					ret = destroyMatrix( &(arKeyword[i].featureMatrix) );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &(arKeyword[i].featureNorm) );
					AUD_ASSERT( ret == 0 );
				}
			}
		}

		ret = benchmark_finalizethreshold( hBenchmark );
		AUD_ASSERT( ret == 0 );
	}

	ret = benchmark_free( &hBenchmark );
	AUD_ASSERT( ret == 0 );

	keywordBufLen  = 0;
	free( pKeywordBuf );
	pKeywordBuf = NULL;

	AUDLOG( "DTW-MFCC Multi-Enrollment Benchmark finished\n" );

	return 0;
}

