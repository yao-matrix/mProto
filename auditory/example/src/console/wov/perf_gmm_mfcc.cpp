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

#include "det.h"

#define ENBALE_FRAMEADMISSION 0


AUD_Int32s perf_gmm_mfcc()
{
	AUD_Error  error          = AUD_ERROR_NONE;
	AUD_Int8s  inputPath[256] = { 0, };

	AUD_Int32s ret, data;
	AUD_Int32s i;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	dir   *pKeywordDir   = openDir( (const char*)WOV_KEYWORD_GMMMODEL_DIR );
	entry *pKeywordEntry = NULL;
	while ( ( pKeywordEntry = scanDir( pKeywordDir ) ) )
	{
		if ( strstr( pKeywordEntry->name, ".gmm" ) == NULL )
		{
			continue;
		}

		char keywordFile[256] = { 0, };
		snprintf( keywordFile, 256, "%s/%s", (const char*)WOV_KEYWORD_GMMMODEL_DIR, pKeywordEntry->name );

		void *hKeywordGmm = NULL;
		FILE *fpKeyword = fopen( keywordFile, "rb" );
		if ( fpKeyword == NULL )
		{
			AUDLOG( "cannot open gmm model file: [%s]\n", keywordFile );
			return AUD_ERROR_IOFAILED;
		}

		AUDLOG( "import speaker model from: [%s]\n", keywordFile );
		error = gmm_import( &hKeywordGmm, fpKeyword );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpKeyword );
		fpKeyword = NULL;

		// AUDLOG( "keyword model is(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
		// gmm_show( hKeywordGmm );

		char keywordName[256] = { 0, };
		char *pToken = strrchr( (char*)pKeywordEntry->name, (int)'.' );
		AUD_ASSERT( pToken );

		size_t len = 0;
		while ( pToken > (char*)pKeywordEntry->name )
		{
			len++;
			pToken--;
		}
		AUD_ASSERT( len > 0 );

		snprintf( keywordName, len + 1, "%s", pKeywordEntry->name );

		char imposterFile[256] = { 0, };
		snprintf( imposterFile, 256, "%s/%s-imposter.gmm", (const char*)WOV_IMPOSTER_GMMMODEL_DIR, keywordName );

		// load imposter
		AUDLOG( "import imposter from: [%s]\n", imposterFile );
		void *hImposter = NULL;
		FILE *fpImposter = fopen( imposterFile, "rb" );
		if ( fpImposter == NULL )
		{
			AUDLOG( "cannot open gmm model file: [%s]\n", imposterFile );
			return AUD_ERROR_IOFAILED;
		}
		error = gmm_import( &hImposter, fpImposter );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpImposter );
		fpImposter = NULL;

		void *detHandle = NULL;
		det_init( &detHandle, (const char*)pKeywordEntry->name );

		// extract input stream feature & recognize
		dir        *pDir     = openDir( (char*)inputPath );
		entry      *pEntry   = NULL;
		AUD_Int32s sampleNum = 0;

		AUD_Int16s    *pBuf = NULL;
		AUD_Int32s    bufLen;

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
			AUDLOG( "input stream: %s, keyword: %s\n", inputStream, pKeywordEntry->name );

			bufLen = fileSummary.dataChunkBytes;
			pBuf   = (AUD_Int16s*)calloc( bufLen + 100, 1 );
			AUD_ASSERT( pBuf );

			sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
			AUD_ASSERT( sampleNum > 0 );

			// front end processing
			 // pre-emphasis
			sig_preemphasis( pBuf, pBuf, sampleNum );

			AUD_Int32s j           = 0;
			AUD_Int32s frameStride = 0;
#if ( ENBALE_FRAMEADMISSION )
			// NOTE: frame admission
			AUD_Int32s validBufLen = sampleNum / FRAME_STRIDE * FRAME_LEN * BYTES_PER_SAMPLE;
			AUD_Int16s *pValidBuf  = (AUD_Int16s*)calloc( validBufLen, 1 );
			AUD_ASSERT( pValidBuf );

			// AUDLOG( "input stream: %s, ", fileName );
			AUD_Int32s validSampleNum = 0;
			validSampleNum = sig_frameadmit( pBuf, sampleNum, pValidBuf );

			j           = validSampleNum / FRAME_LEN;
			frameStride = FRAME_LEN;

			AUD_ASSERT( (j - MFCC_DELAY) > 10 );
#else
			for ( j = 0; j * FRAME_STRIDE + FRAME_LEN < sampleNum; j++ )
			{
				;
			}
			frameStride = FRAME_STRIDE;
#endif

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

			// init mfcc handle
			error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
			AUD_ASSERT( error == AUD_ERROR_NONE );

#if ( ENBALE_FRAMEADMISSION )
			error = mfcc16s32s_calc( hMfccHandle, pValidBuf, validSampleNum, &feature );
			AUD_ASSERT( error == AUD_ERROR_NONE );
#else
			error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
			AUD_ASSERT( error == AUD_ERROR_NONE );
#endif

			// calc gmm scores
			AUD_Double imposterScore = 0., keywordScore = 0.;
			AUD_Double frameScore    = 0., finalScore   = 0.;

			AUD_Vector compLlr;
			compLlr.len      = gmm_getmixnum( hImposter );
			compLlr.dataType = AUD_DATATYPE_DOUBLE;
			ret = createVector( &compLlr );
			AUD_ASSERT( ret == 0 );

			AUD_Vector bestIndex;
			bestIndex.len      = 10;
			bestIndex.dataType = AUD_DATATYPE_INT32S;
			ret = createVector( &bestIndex );
			AUD_ASSERT( ret == 0 );

			for ( i = 0; i < feature.featureMatrix.rows; i++ )
			{
				frameScore = gmm_llr( hImposter, &(feature.featureMatrix), i, &compLlr );

				// get top N gaussian component
				// showVector( &compLlr );
				// ret = sortVector( &compLlr, &bestIndex );
				// AUD_ASSERT( ret == 0 );
				// showVector( &bestIndex );

				// frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
				// for( j = 1; j < bestIndex.len; j++ )
				// {
				//		frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
				// }
				imposterScore += frameScore;

				frameScore = gmm_llr( hKeywordGmm, &(feature.featureMatrix), i, &compLlr );

				// frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
				// for( j = 1; j < bestIndex.len; j++ )
				// {
				//		frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
				// }

				keywordScore += frameScore;
			}
			keywordScore  /= feature.featureMatrix.rows;
			imposterScore /= feature.featureMatrix.rows;

			finalScore = keywordScore - imposterScore;
			AUDLOG( "\tkeyword score: %.2f, imposter score: %.2f, final score: %.2f\n", keywordScore, imposterScore, finalScore );

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

#if ( ENBALE_FRAMEADMISSION )
			free( pValidBuf );
			pValidBuf   = NULL;
			validBufLen = 0;
#endif

			AUDLOG( "\n" );
		}
		closeDir( pDir );
		pDir = NULL;

		error = gmm_free( &hKeywordGmm );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = gmm_free( &hImposter );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		det_free( &detHandle );
	}
	closeDir( pKeywordDir );
	pKeywordDir = NULL;

	AUDLOG( "GMM-MFCC Benchmark finished\n" );

	return 0;
}
