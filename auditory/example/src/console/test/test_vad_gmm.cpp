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

// TODO: add model file loaction and name here
#define SPEECH_MODEL_FILE     "./speech.gmm"
#define BACKGROUND_MODEL_FILE "./background.gmm"

AUD_Int32s test_vad_gmm()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data, ret, i, j;
	AUD_Error       error;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	AUDLOG( "\n\n" );

	// load speech GMM model
	AUDLOG( "import speech gmm model from: [%s]\n", SPEECH_MODEL_FILE );
	void *hSpeechGmm = NULL;
	FILE *fpSpeechGmm = fopen( (char*)SPEECH_MODEL_FILE, "rb" );
	if ( fpSpeechGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", SPEECH_MODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hSpeechGmm, fpSpeechGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpSpeechGmm );
	fpSpeechGmm = NULL;

	// load background GMM model
	AUDLOG( "import background gmm model from: [%s]\n", BACKGROUND_MODEL_FILE );
	void *hBackgroundGmm = NULL;
	FILE *fpBackgroundGmm = fopen( (char*)BACKGROUND_MODEL_FILE, "rb" );
	if ( fpBackgroundGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", BACKGROUND_MODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hBackgroundGmm, fpBackgroundGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpBackgroundGmm );
	fpBackgroundGmm = NULL;

	// file & directory operation, linux dependent
	entry      *pEntry         = NULL;
	dir        *pDir           = NULL;
	AUD_Int8s  fileName[256]   = { 0, };
	AUD_Int32s sampleNum;

	AUD_Int32s bufLen          = 0;
	AUD_Int16s *pBuf           = NULL;

	AUD_Int32s frameStride;
	FILE       *fp = fopen( "./frame_score.log", "wb" );
	AUD_ASSERT( fp );

	pDir = openDir( (const char*)inputPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Summary fileSummary;

		snprintf( (char*)fileName, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( fileName, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		bufLen = fileSummary.dataChunkBytes;
		pBuf   = (AUD_Int16s*)calloc( bufLen, 1 );
		AUD_ASSERT( pBuf );

		sampleNum = readWavFromFile( fileName, pBuf, bufLen );
		if ( sampleNum < 0 )
		{
			continue;
		}

		// pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );

		AUD_Int32s frameNum = sampleNum / FRAME_STRIDE;
		AUD_Double *pEnergy = (AUD_Double*)calloc( frameNum * sizeof(AUD_Double), 1 );
		AUD_ASSERT( pEnergy );

		AUD_Int16s *pFrameBuf = NULL;
		AUD_Double maxEnergy = 0.;
		for ( i = 0; i * FRAME_STRIDE + FRAME_LEN < sampleNum; i++ )
		{
			pFrameBuf  = pBuf + i * FRAME_STRIDE;
			pEnergy[i] = 0.;
			for ( j = 0; j < FRAME_LEN; j++ )
			{
				pEnergy[i] += pFrameBuf[j] * pFrameBuf[j];
			}
			if ( maxEnergy < pEnergy[i] )
			{
				maxEnergy = pEnergy[i];
			}
		}
		frameNum = i;
		AUDLOG( "stream: %s, valid frame number: %d\n", fileName, frameNum );
		fprintf( fp, "\n\nstream: %s, valid frame number: %d\n", fileName, frameNum );

		frameStride = FRAME_STRIDE;

		// init mfcc handle
		void *hMfccHandle = NULL;
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		AUD_Feature feature;
		feature.featureMatrix.rows     = 1;
		feature.featureMatrix.cols     = MFCC_FEATDIM;
		feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(feature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		feature.featureNorm.len      = 1;
		feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// streaming handle
		AUD_Double speechScore     = 0.;
		AUD_Double backgroundScore = 0.;

		// TODO: add segment based scoring mechanism
		for ( i = 0; i < frameNum; i++ )
		{
			AUD_Bool isSilence = AUD_FALSE;

			speechScore     = -100.;
			backgroundScore = -50.;

			pFrameBuf = pBuf + i * FRAME_STRIDE;
			if ( pEnergy[i] < 0.001 * maxEnergy )
			{
				// silence
				isSilence = AUD_TRUE;
				goto exit;
			}

			// calc mfcc feature
			error = mfcc16s32s_calc( hMfccHandle, pFrameBuf, FRAME_LEN, &feature );
			AUD_ASSERT( error == AUD_ERROR_NONE || error == AUD_ERROR_MOREDATA );

			// NOTE: normalize feature
			// for ( j = 0; j < feature.featureMatrix.cols; j++ )
			// {
			//	feature.featureMatrix.pData.pInt32s[j] = (AUD_Int32s)round( feature.featureMatrix.pData.pInt32s[j] / (AUD_Double)feature.featureNorm.pData.pInt64s[0] * 65536. );
			// }

			if ( i >= MFCC_DELAY )
			{
				speechScore     = gmm_llr( hSpeechGmm, &(feature.featureMatrix), 0, NULL );
				backgroundScore = gmm_llr( hBackgroundGmm, &(feature.featureMatrix), 0, NULL );
			}

exit:
			fprintf( fp, "\t %d: %.2f, %.2f, %.2f\n", i, speechScore, backgroundScore, speechScore - backgroundScore );
		}

		// mfcc deinit
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		free( pBuf );
		pBuf    = NULL;
		bufLen  = 0;
		free( pEnergy );
		pEnergy = NULL;
	}
	closeDir( pDir );
	pDir = NULL;

	fclose( fp );
	fp = NULL;

	// free gmm model
	error = gmm_free( &hSpeechGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmm_free( &hBackgroundGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "GMM VAD test finished\n" );

	return 0;
}
