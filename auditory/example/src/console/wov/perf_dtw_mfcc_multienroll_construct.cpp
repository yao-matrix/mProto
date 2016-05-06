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
#include <math.h>

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

AUD_Int32s perf_dtw_mfcc_multienroll_construct()
{
	const char      *pInputPath   = "./data/wov/test";
	const char      *pKeywordPath = "./data/wov/keyword_sk";
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
	entry           *pEntry         = NULL;
	dir             *pDir           = NULL;

	AUD_Int32s      i, j, m;

	AUD_Feature     keywordFeature;
	AUD_Int32s      keywordID = 0;
	AUD_Int8s       keywordFile[256];
	AUD_Int8s       keywordName[256];

	AUD_DTWSession  dtwSession;
	AUD_Double      score = 0.;

	AUD_Int32s      sampleNum;
	AUD_Int32s      keywordBufLen = 0;
	AUD_Int16s      *pKeywordBuf  = NULL;
	AUD_Int32s      bufLen        = 0;
	AUD_Int16s      *pBuf         = NULL;

	AUD_Int32s      ret;
	AUD_Int32s      speakerID;

	keywordBufLen  = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
	pKeywordBuf = (AUD_Int16s*)malloc( keywordBufLen );
	AUD_ASSERT( pKeywordBuf != NULL );

	// init performance benchmark
	void *hBenchmark  = NULL;
	char logName[256] = { 0, };
	snprintf( logName, 256, "dtw-mfcc-constructive-multienroll-%d-%d-%d-%d", WOV_DTW_TYPE, WOV_DISTANCE_METRIC, WOV_DTWTRANSITION_STYLE, WOV_DTWSCORING_METHOD );
	ret = benchmark_init( &hBenchmark, (AUD_Int8s*)logName, keywords, keywordNum );
	AUD_ASSERT( ret == 0 );

	for ( threshold = minThreshold; threshold < maxThreshold + stepThreshold; threshold += stepThreshold )
	{
		ret = benchmark_newthreshold( hBenchmark, threshold );
		AUD_ASSERT( ret == 0 );

		for ( keywordID = 0; keywordID < keywordNum; keywordID++ )
		{
			// keyword feature extraction
			for ( speakerID = 1; speakerID <= speakerNum; speakerID++ )
			{
				AUDLOG( "***********************************************\n" );
				AUDLOG( "keyword training start...\n" );

				AUD_DTWSession  arDtwSession[ENROLLMENT_NUM];
				AUD_Feature     arKeyword[ENROLLMENT_NUM];
				AUD_Int32s      arKeywordSampleNum[ENROLLMENT_NUM] = { 0, };
				AUD_Int32s      arKeywordWinNum[ENROLLMENT_NUM]    = { 0, };

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

					void *hTemplateMfccHandle = NULL;

					// front end processing
					 // pre-emphasis
					sig_preemphasis( pKeywordBuf, pKeywordBuf, arKeywordSampleNum[i] );


					// init mfcc handle
					error = mfcc16s32s_init( &hTemplateMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// calc MFCC feature
					error = mfcc16s32s_calc( hTemplateMfccHandle, pKeywordBuf, arKeywordSampleNum[i], &(arKeyword[i]) );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// deinit mfcc handle
					error = mfcc16s32s_deinit( &hTemplateMfccHandle );
					AUD_ASSERT( error == AUD_ERROR_NONE );
				}

				// keyword quality evaluation
				AUD_Double arQuality[ENROLLMENT_NUM][ENROLLMENT_NUM];
				AUD_Vector arPath[ENROLLMENT_NUM][ENROLLMENT_NUM];
				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					for ( j = 0; j < ENROLLMENT_NUM; j++ )
					{
						if ( j == i )
						{
							arQuality[i][j]            = 0.;
							arPath[i][j].pInt32s = NULL;
							continue;
						}

						arPath[i][j].len      = arKeyword[i].featureMatrix.rows;
						arPath[i][j].dataType = AUD_DATATYPE_INT32S;
						ret = createVector( &(arPath[i][j]) );
						AUD_ASSERT( ret == 0 );

						for ( m = 0; m < arPath[i][j].len; m++ )
						{
							(arPath[i][j].pInt32s)[m] = -1;
						}

						arDtwSession[i].dtwType        = AUD_DTWTYPE_CLASSIC;
						arDtwSession[i].distType       = WOV_DISTANCE_METRIC;
						arDtwSession[i].transitionType = WOV_DTWTRANSITION_STYLE;
						error = dtw_initsession( &(arDtwSession[i]), &(arKeyword[i]), arKeyword[j].featureMatrix.rows );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						// update frame cost
						error = dtw_updatefrmcost( &(arDtwSession[i]), &(arKeyword[j]) );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						// compute DTW
						error = dtw_match( &(arDtwSession[i]), AUD_DTWSCORE_TERMINAL, &(arQuality[i][j]), &(arPath[i][j]) );
						AUD_ASSERT( error == AUD_ERROR_NONE );

						error = dtw_deinitsession( &(arDtwSession[i]) );
						AUD_ASSERT( error == AUD_ERROR_NONE );
					}
				}

				snprintf( (char*)keywordName, 256, "%d-%s-", speakerID, keywords[keywordID] );

				AUDLOG( "template[%s] quality matrix:\n", (char*)keywordName );
				AUD_Double bestQuality  = 3.;
				AUD_Int32s bestTemplate = -1;
				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					AUD_Double quality = 0.;
					for ( j = 0; j < ENROLLMENT_NUM; j++ )
					{
						AUDLOG( "%.3f, ", arQuality[i][j] );
						quality += arQuality[i][j];
					}
					if ( quality < bestQuality )
					{
						bestQuality  = quality;
						bestTemplate = i;
					}
					AUDLOG( "\t template len: %d\n", arKeyword[i].featureMatrix.rows );
				}
				AUDLOG( "best template: %d\n", bestTemplate );
				AUDLOG( "template quality matrix done\n" );

				// construct final template
				keywordFeature.featureMatrix.rows     = arKeyword[bestTemplate].featureMatrix.rows;
				keywordFeature.featureMatrix.cols     = MFCC_FEATDIM;
				keywordFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
				ret = createMatrix( &(keywordFeature.featureMatrix) );
				AUD_ASSERT( ret == 0 );

				keywordFeature.featureNorm.len       = arKeyword[bestTemplate].featureMatrix.rows;
				keywordFeature.featureNorm.dataType  = AUD_DATATYPE_INT64S;
				ret = createVector( &(keywordFeature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				// XXX
				AUD_Double pWeight0, pWeight1, pWeight2;
				pWeight0 = 2.;
				pWeight1 = 2. - arQuality[bestTemplate][(bestTemplate + 1) % ENROLLMENT_NUM];
				pWeight2 = 2. - arQuality[bestTemplate][(bestTemplate + 2) % ENROLLMENT_NUM];
				AUD_Double sum = pWeight0 + pWeight1 + pWeight2;
				pWeight0 /= sum;
				pWeight1 /= sum;
				pWeight2 /= sum;

				AUD_Matrix *pMainMatrix    = &(arKeyword[bestTemplate].featureMatrix);
				AUD_Matrix *pAssistMatrix1 = &(arKeyword[(bestTemplate + 1) % ENROLLMENT_NUM].featureMatrix);
				AUD_Matrix *pAssistMatrix2 = &(arKeyword[(bestTemplate + 2) % ENROLLMENT_NUM].featureMatrix);
				AUD_Int32s *pPath1         = arPath[bestTemplate][(bestTemplate + 1) % ENROLLMENT_NUM].pInt32s;
				AUD_Int32s *pPath2         = arPath[bestTemplate][(bestTemplate + 2) % ENROLLMENT_NUM].pInt32s;
				for ( i = 0; i < keywordFeature.featureMatrix.rows; i++ )
				{
					AUD_Int32s *pRes     = keywordFeature.featureMatrix.pInt32s + i * keywordFeature.featureMatrix.cols;
					AUD_Int32s *pFactor0 = pMainMatrix->pInt32s + i * pMainMatrix->cols;
					AUD_Int32s *pFactor1 = NULL;
					AUD_Int32s *pFactor2 = NULL;
					if ( pPath1[i] != -1 )
					{
						pFactor1 = pAssistMatrix1->pInt32s +  pPath1[i] * pAssistMatrix1->cols;
					}
					else
					{
						pFactor1 = pMainMatrix->pInt32s + i * pMainMatrix->cols;
					}

					if ( pPath2[i] != -1 )
					{
						pFactor2 = pAssistMatrix2->pInt32s +  pPath2[i] * pAssistMatrix2->cols;
					}
					else
					{
						pFactor2 = pMainMatrix->pInt32s + i * pMainMatrix->cols;
					}

					AUD_Double tmp = 0.;
					for ( j = 0; j < keywordFeature.featureMatrix.cols; j++ )
					{
						pRes[j] = (AUD_Int32s)round( pWeight0 * pFactor0[j] + pWeight1 * pFactor1[j] + pWeight2 * pFactor2[j] );
						tmp += (AUD_Double)pRes[j] * pRes[j];
					}
					(keywordFeature.featureNorm.pInt64s)[i] = (AUD_Int64s)sqrt( tmp );
				}

				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					for ( j = 0; j < ENROLLMENT_NUM; j++ )
					{
						ret = destroyVector( &(arPath[i][j]) );
					}
				}

				for ( i = 0; i < ENROLLMENT_NUM; i++ )
				{
					ret = destroyMatrix( &(arKeyword[i].featureMatrix) );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &(arKeyword[i].featureNorm) );
					AUD_ASSERT( ret == 0 );
				}

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

					bufLen = fileSummary.dataChunkBytes;
					pBuf   = (AUD_Int16s*)malloc( bufLen + 100 );
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
					dtwSession.dtwType        = WOV_DTW_TYPE;
					dtwSession.distType       = WOV_DISTANCE_METRIC;
					dtwSession.transitionType = WOV_DTWTRANSITION_STYLE;
					error = dtw_initsession( &dtwSession, &keywordFeature, inFeature.featureMatrix.rows );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// update frame cost
					error = dtw_updatefrmcost( &dtwSession, &inFeature );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					// compute DTW
					error = dtw_match( &dtwSession, WOV_DTWSCORING_METHOD, &score, NULL );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					error = dtw_deinitsession( &dtwSession );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					if ( score < threshold )
					{
						isRecognized = AUD_TRUE;
						// AUDLOG( "\t!!![%-30s] MATCH pattern[%-10s], score: %.2f\n", pEntry->name, keywordName, score );
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
					ret = benchmark_additem( hBenchmark, (AUD_Int8s*)pEntry->name, (AUD_Double)score, isRecognized );
					AUD_ASSERT( ret == 0 );
				}
				closeDir( pDir );
				pDir = NULL;

				ret = benchmark_finalizetemplate( hBenchmark );
				AUD_ASSERT( ret == 0 );
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

	AUDLOG( "DTW-MFCC Constructive Multi-Enrollment Benchmark finished\n" );

	return 0;
}

