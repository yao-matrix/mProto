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
#include <ctype.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"

#include "math/mmath.h"

#include "det.h"

#define TNORM_IMPOSTER_NUM 10
#define TOP_N              5

AUD_Int32s sv_gmm()
{
	AUD_Error  error          = AUD_ERROR_NONE;
	AUD_Int8s  inputPath[256] = { 0, };

	AUD_Int32s ret, data;
	AUD_Int32s i, k;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	// read UBM model from file
	void *hUbm = NULL;
	FILE *fpUbm = fopen( SV_UBM_GMMMODEL_FILE, "rb" );
	if ( fpUbm == NULL )
	{
		AUDLOG( "cannot open ubm model file: [%s]\n", SV_UBM_GMMMODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hUbm, fpUbm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpUbm );
	fpUbm = NULL;

	dir   *pSpeakerDir   = openDir( (const char*)SV_SPEAKER_GMMMODEL_DIR );
	entry *pSpeakerEntry = NULL;
	while ( ( pSpeakerEntry = scanDir( pSpeakerDir ) ) )
	{
		if ( strstr( pSpeakerEntry->name, ".gmm" ) == NULL )
		{
			continue;
		}

		char speakerFile[256] = { 0, };
		snprintf( speakerFile, 256, "%s/%s", (const char*)SV_SPEAKER_GMMMODEL_DIR, pSpeakerEntry->name );

		void *hSpeakerGmm = NULL;
		FILE *fpSpeaker   = fopen( speakerFile, "rb" );
		if ( fpSpeaker == NULL )
		{
			AUDLOG( "cannot open gmm model file: [%s]\n", speakerFile );
			return AUD_ERROR_IOFAILED;
		}

		AUDLOG( "import speaker model from: [%s]\n", speakerFile );
		error = gmm_import( &hSpeakerGmm, fpSpeaker );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpSpeaker );
		fpSpeaker = NULL;

		// AUDLOG( "speaker model is(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
		// gmm_show( hSpeakerGmm );

		// T-Norm imposter models
		void          *hImposterGmm[TNORM_IMPOSTER_NUM]  = { NULL, };
		AUD_Double    imposterScores[TNORM_IMPOSTER_NUM] = { 0., };
		dir           *pImposterDir   = openDir( (const char*)SV_SPEAKER_GMMMODEL_DIR );
		entry         *pImposterEntry = NULL;
		AUD_Int32s    imposterNum     = 0;
		while ( ( pImposterEntry = scanDir( pImposterDir ) ) && imposterNum < TNORM_IMPOSTER_NUM )
		{
			if ( strstr( pImposterEntry->name, ".gmm" ) == NULL || !strcmp( pImposterEntry->name, pSpeakerEntry->name ) )
			{
				continue;
			}

			char imposterFile[256] = { 0, };
			snprintf( imposterFile, 256, "%s/%s", (const char*)SV_SPEAKER_GMMMODEL_DIR, pImposterEntry->name );

			FILE *fpImposter  = fopen( imposterFile, "rb" );
			if ( fpImposter == NULL )
			{
				AUDLOG( "cannot open gmm model file: [%s]\n", imposterFile );
				return AUD_ERROR_IOFAILED;
			}

			AUDLOG( "import imposter[%d] model from: [%s]\n", imposterNum, imposterFile );
			error = gmm_import( &(hImposterGmm[imposterNum]), fpImposter );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			fclose( fpImposter );
			fpImposter = NULL;

			imposterNum++;
		}
		closeDir( pImposterDir );
		pImposterDir   = NULL;
		pImposterEntry = NULL;

		void *detHandle = NULL;
		det_init( &detHandle, (const char*)pSpeakerEntry->name );

		// extract input stream feature & recognize
		AUD_Int32s    sampleNum = 0;

		AUD_Int16s    *pBuf     = NULL;
		AUD_Int32s    bufLen    = 0;

		dir   *pDir      = NULL;
		entry *pEntry    = NULL;
		entry *pSubEntry = NULL;
		dir   *pSubDir   = NULL;

		pDir = openDir( (char*)inputPath );
		while ( ( pEntry = scanDir( pDir ) ) )
		{
			if ( pEntry->type == FILE_TYPE_DIRECTORY && pEntry->name[0] != '.' ) // a visible folder
			{
				char dirName[256] = { 0, };
				snprintf( dirName, 256, "%s/%s", (char*)inputPath, pEntry->name );

				pSubDir = openDir( (const char*)dirName );
				while ( ( pSubEntry = scanDir( pSubDir ) ) )
				{
					AUD_Int8s   inputStream[256] = { 0, };
					void        *hMfccHandle     = NULL;
					AUD_Summary fileSummary;

					snprintf( (char*)inputStream, 256, "%s/%s", (const char*)dirName, pSubEntry->name );

					ret = parseWavFromFile( inputStream, &fileSummary );
					if ( ret < 0 )
					{
						continue;
					}
					AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

					bufLen = fileSummary.dataChunkBytes;
					pBuf   = (AUD_Int16s*)calloc( bufLen + 100, 1 );
					AUD_ASSERT( pBuf );

					sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
					AUD_ASSERT( sampleNum > 0 );

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

					// calc gmm scores
					AUD_Double ubmScore   = 0., speakerScore = 0.;
					AUD_Double frameScore = 0., finalScore   = 0.;

					AUD_Vector compLlr;
					compLlr.len      = gmm_getmixnum( hUbm );
					compLlr.dataType = AUD_DATATYPE_DOUBLE;
					ret = createVector( &compLlr );
					AUD_ASSERT( ret == 0 );

					AUD_Vector bestIndex;
					bestIndex.len      = TOP_N;
					bestIndex.dataType = AUD_DATATYPE_INT32S;
					ret = createVector( &bestIndex );
					AUD_ASSERT( ret == 0 );

					for ( i = 0; i < feature.featureMatrix.rows; i++ )
					{
						// calc ubm score
						frameScore = gmm_llr( hUbm, &(feature.featureMatrix), i, &compLlr );

						// get top N gaussian component
						// showVector( &compLlr );
						ret = sortVector( &compLlr, &bestIndex );
						AUD_ASSERT( ret == 0 );
						// showVector( &bestIndex );

						frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
						for( j = 1; j < bestIndex.len; j++ )
						{
							frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
						}
						ubmScore += frameScore;

						// calc speaker score
						frameScore = gmm_llr( hSpeakerGmm, &(feature.featureMatrix), i, &compLlr );

						frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
						for( j = 1; j < bestIndex.len; j++ )
						{
							frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
						}

						speakerScore += frameScore;

						// calc imposter scores
						for ( k = 0; k < imposterNum; k++ )
						{
							frameScore = gmm_llr( hImposterGmm[k], &(feature.featureMatrix), i, &compLlr );

							frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
							for( j = 1; j < bestIndex.len; j++ )
							{
								frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
							}
							imposterScores[k] += frameScore;
						}
					}
					speakerScore /= feature.featureMatrix.rows;
					ubmScore     /= feature.featureMatrix.rows;

					finalScore = speakerScore - ubmScore;
					for ( k = 0; k < imposterNum; k++ )
					{
						imposterScores[k] = imposterScores[k] / feature.featureMatrix.rows - ubmScore;
					}

					// T-Normalization
					AUD_Double mean = 0., std = 0.;
					for ( i = 0; i < imposterNum; i++ )
					{
						mean += imposterScores[i];
					}
					mean /= imposterNum;

					for ( i = 0; i < imposterNum; i++ )
					{
						std += ( imposterScores[i] - mean ) * ( imposterScores[i] - mean );
					}
					std /= imposterNum;
					std = sqrt( std );

					finalScore = (finalScore - mean) / std;
					AUDLOG( "input stream: %s, speaker: %s, declarant: %s, score: %.3f\n", inputStream, pEntry->name, pSpeakerEntry->name, finalScore );

					// insert log
					det_additem( detHandle, (AUD_Int8s*)pEntry->name, finalScore );

					// deinit mfcc handle
					error = mfcc16s32s_deinit( &hMfccHandle );
					AUD_ASSERT( error == AUD_ERROR_NONE );

					ret = destroyMatrix( &(feature.featureMatrix) );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &(feature.featureNorm) );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &bestIndex );
					AUD_ASSERT( ret == 0 );

					ret = destroyVector( &compLlr );
					AUD_ASSERT( ret == 0 );

					free( pBuf );
					pBuf   = NULL;
					bufLen = 0;

					AUDLOG( "\n" );
				}
				closeDir( pSubDir );
				pSubDir   = NULL;
				pSubEntry = NULL;
			}
		}
		closeDir( pDir );
		pDir   = NULL;
		pEntry = NULL;

		error = gmm_free( &hSpeakerGmm );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		for ( k = 0; k < imposterNum; k++ )
		{
			error = gmm_free( &(hImposterGmm[k]) );
			AUD_ASSERT( error == AUD_ERROR_NONE );
		}

		det_free( &detHandle );
	}
	closeDir( pSpeakerDir );
	pSpeakerDir = NULL;

	error = gmm_free( &hUbm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "GMM Speaker Verification Benchmark finished\n" );

	return 0;
}
