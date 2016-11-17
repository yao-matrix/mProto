#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>


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

AUD_Int32s perf_hmm_mfcc()
{
	char       inputPath[256] = { 0, };
	AUD_Double threshold, minThreshold, maxThreshold, stepThreshold;
	AUD_Error  error        = AUD_ERROR_NONE;
	AUD_Int32s ret, data;
	AUD_Bool   isRecognized = AUD_FALSE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give input wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "input path is: %s\n", inputPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give min test threshold:\n" );
	data = scanf( "%lf", &minThreshold );
	AUDLOG( "min threshold is: %.2f\n", minThreshold );

	setbuf( stdin, NULL );
	AUDLOG( "pls give max test threshold:\n" );
	data = scanf( "%lf", &maxThreshold );
	AUDLOG( "max threshold is: %.2f\n", maxThreshold );

	setbuf( stdin, NULL );
	AUDLOG( "pls give threshold step:\n" );
	data = scanf( "%lf", &stepThreshold );
	AUDLOG( "threshold step is: %.2f\n", stepThreshold );

	// load grabage model
	AUDLOG( "import garbage gmm model from: [%s]\n", WOV_UBM_GMMHMMMODEL_FILE );
	void *hGarbageGmm = NULL;
	FILE *fpGmm = fopen( (char*)WOV_UBM_GMMHMMMODEL_FILE, "rb" );
	if ( fpGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", WOV_UBM_GMMHMMMODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hGarbageGmm, fpGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpGmm );
	fpGmm = NULL;

	// init performance benchmark
	void *hBenchmark = NULL;
	char logName[256] = { 0, };
	snprintf( logName, 256, "hmm-mfcc-%.2f-%.2f", WOV_GMM_CLUSTER_THRESHOLD, WOV_HMM_MAP_TAU );
	ret = benchmark_init( &hBenchmark, (AUD_Int8s*)logName, keywords, sizeof(keywords) / sizeof(keywords[0]) );
	AUD_ASSERT( ret == 0 );

	for ( threshold = minThreshold; threshold < maxThreshold + stepThreshold; threshold += stepThreshold )
	{
		ret = benchmark_newthreshold( hBenchmark, threshold );
		AUD_ASSERT( ret == 0 );

		dir   *pKeywordDir   = openDir( (const char*)WOV_KEYWORD_GMMHMMMODEL_DIR );
		entry *pKeywordEntry = NULL;
		while ( ( pKeywordEntry = scanDir( pKeywordDir ) ) )
		{
			if ( strstr( pKeywordEntry->name, ".hmm" ) == NULL )
			{
				continue;
			}

			char keywordModel[256] = { 0, };
			char keywordName[256] = { 0, };
			snprintf( (char*)keywordModel, 256, "%s/%s", (const char*)WOV_KEYWORD_GMMHMMMODEL_DIR, pKeywordEntry->name );
			snprintf( (char*)keywordName, 256, "%s", pKeywordEntry->name );

			// load speaker adapted keyword model
			void *hKeywordHmm = NULL;
			error = gmmhmm_import( &hKeywordHmm, (AUD_Int8s*)keywordModel );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// AUDLOG( "speaker model is(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
			// gmmhmm_show( hKeywordHmm );

			ret = benchmark_newtemplate( hBenchmark, (AUD_Int8s*)pKeywordEntry->name );
			AUD_ASSERT( ret == 0 );

			// extract input stream feature & recognize
			dir        *pDir     = openDir( inputPath );
			entry      *pEntry   = NULL;
			AUD_Int32s sampleNum = 0;

			AUD_Int16s    *pBuf = NULL;
			AUD_Int32s    bufLen;

			AUD_Double    speakerScore, garbageScore;
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
				AUDLOG( "input stream: %s, keyword: %s\n", inputStream, keywordName );

				bufLen = fileSummary.dataChunkBytes;
				pBuf   = (AUD_Int16s*)malloc( bufLen + 100 );
				AUD_ASSERT( pBuf );

				sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );

				isRecognized = AUD_FALSE;

				AUD_Int32s j = 0;
				// segment number of the stream
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

				// front end processing
				 // pre-emphasis
				sig_preemphasis( pBuf, pBuf, sampleNum );


				// init mfcc handle
				error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// calc mfcc feature
				error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// calc garbage score
				garbageScore = 0.;
				AUD_Double frameScore = 0.;
				for ( int i = 0; i < feature.featureMatrix.rows; i++ )
				{
					frameScore = gmm_llr( hGarbageGmm, &(feature.featureMatrix), i, NULL );
					garbageScore += frameScore;
				}
				garbageScore /= feature.featureMatrix.rows;

				// calc speaker score
				speakerScore = 0.;
				error = gmmhmm_forward( hKeywordHmm, &(feature.featureMatrix), &speakerScore );
				AUD_ASSERT( error == AUD_ERROR_NONE );
				speakerScore /= feature.featureMatrix.rows;

				AUDLOG( "\tspeaker score: %.2f, garbage score: %.2f, score: %.2f\n", speakerScore, garbageScore, speakerScore - garbageScore );

				if ( speakerScore - garbageScore > threshold )
				{
					AUDLOG( "\t!!!FOUND pattern, score: %.2f\n", speakerScore - garbageScore );
					isRecognized = AUD_TRUE;
				}

				// insert log entry
				ret = benchmark_additem( hBenchmark, (AUD_Int8s*)pEntry->name, speakerScore - garbageScore, isRecognized );
				AUD_ASSERT( ret == 0 );

				// deinit mfcc handle
				error = mfcc16s32s_deinit( &hMfccHandle );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				ret = destroyMatrix( &(feature.featureMatrix) );
				AUD_ASSERT( ret == 0 );

				ret = destroyVector( &(feature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				free( pBuf );
				pBuf   = NULL;
				bufLen = 0;

				AUDLOG( "\n" );
			}
			closeDir( pDir );
			pDir = NULL;

			error = gmmhmm_free( &hKeywordHmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			ret = benchmark_finalizetemplate( hBenchmark );
			AUD_ASSERT( ret == 0 );
		}
		closeDir( pKeywordDir );
		pKeywordDir = NULL;

		ret = benchmark_finalizethreshold( hBenchmark );
		AUD_ASSERT( ret == 0 );
	}

	ret = benchmark_free( &hBenchmark );
	AUD_ASSERT( ret == 0 );

	error = gmm_free( &hGarbageGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "HMM-MFCC Benchmark finished\n" );

	return 0;
}
