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


AUD_Int32s test_hmm_mfcc()
{
	AUD_Int8s  inputPath[256]    = { 0, };
	AUD_Double threshold         = 0.;
	AUD_Error  error             = AUD_ERROR_NONE;
	AUD_Int32s data;

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

	// load speaker adapted keyword model
	void  *hKeywordHmm     = NULL;
	dir   *pKeywordDir     = openDir( (const char*)WOV_KEYWORD_GMMHMMMODEL_DIR );
	entry *pKeywordEntry   = NULL;
	char  keywordName[256] = "0";
	while ( ( pKeywordEntry = scanDir( pKeywordDir ) ) )
	{
		if ( strstr( pKeywordEntry->name, ".hmm" ) == NULL )
		{
			continue;
		}

		char keywordModel[256];
		snprintf( (char*)keywordModel, 256, "%s/%s", (const char*)WOV_KEYWORD_GMMHMMMODEL_DIR, pKeywordEntry->name );
		snprintf( keywordName, 256, "%s", pKeywordEntry->name );

		error = gmmhmm_import( &hKeywordHmm, (AUD_Int8s*)keywordModel );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// AUDLOG( "speaker model is(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
		// gmmhmm_show( hKeywordHmm );

		break;
	}
	closeDir( pKeywordDir );
	pKeywordDir = NULL;

	// extract input stream feature & recognize
	dir         *pDir     = openDir( (const char*)inputPath );
	entry       *pEntry   = NULL;
	AUD_Int32s  sampleNum = 0;
	AUD_Int32s  ret;

	AUD_Int16s  *pBuf = NULL;
	AUD_Int32s  bufLen;

	AUD_Double  speakerScore, garbageScore;

	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Int8s   inputStream[256] = { 0, };
		void        *hMfccHandle     = NULL;
		AUD_Summary fileSummary;

#if 0
		char fName[256] = { 0, };
		snprintf( fName, 256, "./feature%d.txt", cnt );
		FILE *fFeature = fopen( fName, "wb" );
		AUD_ASSERT( fFeature );
		cnt++;
#endif

		snprintf( (char*)inputStream, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( inputStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );
		AUDLOG( "input stream: %s, keyword name: %s\n", inputStream, keywordName );

		bufLen  = fileSummary.dataChunkBytes;
		pBuf = (AUD_Int16s*)calloc( bufLen + 100, 1 );
		AUD_ASSERT( pBuf );

		sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		AUD_Int32s j = 0;
		// frame number of the stream
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

		// pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );

		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc mfcc feature
		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

#if 0
		// show feature
		AUDLOG( "input feature:\n\n" );
		showMatrix( &(feature.featureMatrix) );
#endif

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
		}

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

	error = gmm_free( &hGarbageGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmmhmm_free( &hKeywordHmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "HMM-MFCC test finished\n" );

	return 0;
}
