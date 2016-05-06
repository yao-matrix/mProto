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
#define TOP_N           10

AUD_Int32s wov_adapt_gmm()
{
	AUD_Error  error = AUD_ERROR_NONE;
	AUD_Int32s ret   = 0;
	AUD_Int8s  wavPath[256] = { 0, };
	AUD_Int32s data;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give adapt wav stream's folder path:\n" );
	wavPath[0] = '\0';
	data = scanf( "%s", wavPath );
	AUDLOG( "adapt wav stream's folder path is: %s\n", wavPath );

	// step 1: read UBM model from file
	void *hUbm = NULL;
	FILE *fpUbm = fopen( WOV_UBM_GMMMODEL_FILE, "rb" );
	if ( fpUbm == NULL )
	{
		AUDLOG( "cannot open ubm model file: [%s]\n", WOV_UBM_GMMMODEL_FILE );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hUbm, fpUbm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpUbm );
	fpUbm = NULL;

	// AUDLOG( "ubm GMM as:\n" );
	// gmm_show( hUbm );

	AUD_Int32s keywordID, speakerID;
	AUD_Int32s i = 0, j = 0;

	for ( keywordID = 0; keywordID < keywordNum; keywordID++ )
	{
		for ( speakerID = 1; speakerID <= speakerNum; speakerID++ )
		{
			AUD_Int32s totalWinNum = 0;
			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				AUD_Int8s   keywordFile[256] = { 0, };
				AUD_Summary fileSummary;
				AUD_Int32s  sampleNum = 0;

				snprintf( (char*)keywordFile, 256, "%s/%s/%d-%s-%d_16k.wav", wavPath, keywords[keywordID], speakerID, keywords[keywordID], i + 1 );
				// AUDLOG( "%s\n", keywordFile );

				ret = parseWavFromFile( keywordFile, &fileSummary );
				if ( ret < 0 )
				{
					continue;
				}
				AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

				// request memeory for template
				sampleNum = fileSummary.dataChunkBytes / fileSummary.bytesPerSample;
				for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
				{
					;
				}
				j = j - MFCC_DELAY;

				totalWinNum += j;
			}

			AUD_Matrix featureMatrix;
			featureMatrix.rows     = totalWinNum;
			featureMatrix.cols     = MFCC_FEATDIM;
			featureMatrix.dataType = AUD_DATATYPE_INT32S;
			ret = createMatrix( &featureMatrix );
			AUD_ASSERT( ret == 0 );

			AUD_Int32s currentRow  = 0;
			for ( i = 0; i < ENROLLMENT_NUM; i++ )
			{
				AUD_Int8s   keywordFile[256] = { 0, };
				AUD_Summary fileSummary;
				AUD_Int32s  sampleNum        = 0;
				void        *hMfccHandle     = NULL;

				snprintf( (char*)keywordFile, 256, "%s/%s/%d-%s-%d_16k.wav", wavPath, keywords[keywordID], speakerID, keywords[keywordID], i + 1 );
				// AUDLOG( "%s\n", keywordFile );

				ret = parseWavFromFile( keywordFile, &fileSummary );
				if ( ret < 0 )
				{
					continue;
				}
				AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

				AUD_Int32s bufLen = fileSummary.dataChunkBytes;
				AUD_Int16s *pBuf  = (AUD_Int16s*)calloc( bufLen, 1 );
				AUD_ASSERT( pBuf );

				sampleNum = readWavFromFile( (AUD_Int8s*)keywordFile, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );

				// pre-processing

				 // pre-emphasis
				sig_preemphasis( pBuf, pBuf, sampleNum );

				 // calc framing number
				for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
				{
					;
				}

				 // XXX: select salient frames
				AUD_Feature feature;
				feature.featureMatrix.rows     = j - MFCC_DELAY;
				feature.featureMatrix.cols     = MFCC_FEATDIM;
				feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
				feature.featureMatrix.pInt32s  = featureMatrix.pInt32s + currentRow * feature.featureMatrix.cols;

				feature.featureNorm.len      = j - MFCC_DELAY;
				feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
				ret = createVector( &(feature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = mfcc16s32s_deinit( &hMfccHandle );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				free( pBuf );
				pBuf   = NULL;
				bufLen = 0;

				ret = destroyVector( &(feature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				currentRow += feature.featureMatrix.rows;
			}

			AUD_Matrix llrMatrix;
			llrMatrix.rows     = totalWinNum;
			llrMatrix.cols     = gmm_getmixnum( hUbm );
			llrMatrix.dataType = AUD_DATATYPE_DOUBLE;
			ret = createMatrix( &llrMatrix );
			AUD_ASSERT( ret == 0 );

			AUD_Double llr = 0.;
			for ( j = 0; j < featureMatrix.rows; j++ )
			{
				AUD_Vector componentLLR;
				componentLLR.len      = llrMatrix.cols;
				componentLLR.dataType = AUD_DATATYPE_DOUBLE;
				componentLLR.pDouble  = llrMatrix.pDouble + j * llrMatrix.cols;

				llr = gmm_llr( hUbm, &(featureMatrix), j, &componentLLR );
			}

			AUD_Vector sumLlr;
			sumLlr.len      = llrMatrix.cols;
			sumLlr.dataType = AUD_DATATYPE_DOUBLE;
			ret = createVector( &sumLlr );
			AUD_ASSERT( ret == 0 );

			AUD_Double *pSumLlr = sumLlr.pDouble;
			for ( j = 0; j < llrMatrix.cols; j++ )
			{
				pSumLlr[j] = llrMatrix.pDouble[j];
			}
			for ( i = 1; i < llrMatrix.rows; i++ )
			{
				for ( j = 0; j < llrMatrix.cols; j++ )
				{
					pSumLlr[j] = logadd( pSumLlr[j],  *(llrMatrix.pDouble + i * llrMatrix.cols + j) );
				}
			}

#if 0
			AUD_Vector bestIndex;
			bestIndex.len      = TOP_N;
			bestIndex.dataType = AUD_DATATYPE_INT32S;
			ret = createVector( &bestIndex );
			AUD_ASSERT( ret == 0 );

			// get top TOP_N component
			ret = sortVector( &sumLlr, &bestIndex );
			AUD_ASSERT( ret == 0 );
#else
			llr = pSumLlr[0];
			for ( j = 1; j < sumLlr.len; j++ )
			{
				llr = logadd( llr, pSumLlr[j] );
			}
			AUDLOG( "llr: %.f\n", llr );

			AUD_Vector sortIndex;
			sortIndex.len      = sumLlr.len;
			sortIndex.dataType = AUD_DATATYPE_INT32S;
			ret = createVector( &sortIndex );
			AUD_ASSERT( ret == 0 );

			ret = sortVector( &sumLlr, &sortIndex );
			AUD_ASSERT( ret == 0 );

			int    num = 0;
			double val = 0.;
			for ( i = 0; i < sortIndex.len; i++ )
			{
				// ln( 0.001 ) ~= -7.
				val =  pSumLlr[sortIndex.pInt32s[i]] - llr + 7.;
				// AUDLOG( "%f, \n", val );
				if ( val < 0 )
				{
					break;
				}
				num++;
			}
			// AUDLOG( "\n" );
			AUD_ASSERT( num > 0 );

			AUDLOG( "computed component num: %d\n", num );

			num = AUD_MAX( num, TOP_N );
			AUDLOG( "normalized component num: %d\n", num );

			AUD_Vector bestIndex;
			bestIndex.len      = num;
			bestIndex.dataType = AUD_DATATYPE_INT32S;
			bestIndex.pInt32s  = sortIndex.pInt32s;
#endif

			// select imposter GMM
			void      *hImposterGmm        = NULL;
			AUD_Int8s imposterGmmName[256] = { 0, };
			snprintf( (char*)imposterGmmName, 256, "%d-%s-imposter", speakerID, keywords[keywordID] );
			error = gmm_select( &hImposterGmm, hUbm, &bestIndex, 0 | GMM_INVERTSELECT_MASK, imposterGmmName );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			gmm_show( hImposterGmm );

			// export gmm
			char imposterFile[256] = { 0 };
			snprintf( imposterFile, 256, "%s/%d-%s-imposter.gmm", WOV_IMPOSTER_GMMMODEL_DIR, speakerID, keywords[keywordID] );
			// AUDLOG( "Export imposter GMM Model to: %s\n", imposterFile );
			FILE *fpImposterGmm = fopen( imposterFile, "wb" );
			AUD_ASSERT( fpImposterGmm );

			error = gmm_export( hImposterGmm, fpImposterGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			// AUDLOG( "Export imposter GMM Model File Done\n" );

			fclose( fpImposterGmm );
			fpImposterGmm = NULL;

			error = gmm_free( &hImposterGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// select keyword GMM
			void      *hAdaptedGmm        = NULL;
			AUD_Int8s adaptedGmmName[256] = { 0, };
			snprintf( (char*)adaptedGmmName, 256, "%d-%s", speakerID, keywords[keywordID] );
			AUDLOG( "%s\n", adaptedGmmName );
			error = gmm_select( &hAdaptedGmm, hUbm, &bestIndex, 0, adaptedGmmName );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			ret = destroyVector( &sumLlr );
			AUD_ASSERT( ret == 0 );

			ret = destroyMatrix( &llrMatrix );
			AUD_ASSERT( ret == 0 );

#if 0
			ret = destroyVector( &bestIndex );
			AUD_ASSERT( ret == 0 );
#else
			ret = destroyVector( &sortIndex );
			AUD_ASSERT( ret == 0 );
#endif

			// adapt GMM
			error = gmm_adapt( hAdaptedGmm, &featureMatrix );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			gmm_show( hAdaptedGmm );

			// export gmm
			char modelFile[256] = { 0 };
			snprintf( modelFile, 256, "%s/%d-%s.gmm", WOV_KEYWORD_GMMMODEL_DIR, speakerID, keywords[keywordID] );
			// AUDLOG( "Export GMM Model to: %s\n", modelFile );
			FILE *fpGmm = fopen( modelFile, "wb" );
			AUD_ASSERT( fpGmm );

			error = gmm_export( hAdaptedGmm, fpGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			// AUDLOG( "Export GMM Model File Done\n" );

			fclose( fpGmm );
			fpGmm = NULL;

			ret = destroyMatrix( &featureMatrix );
			AUD_ASSERT( ret == 0 );

			error = gmm_free( &hAdaptedGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
		}
	}

	error = gmm_free( &hUbm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "GMM model adapt done\n" );

	return 0;
}


