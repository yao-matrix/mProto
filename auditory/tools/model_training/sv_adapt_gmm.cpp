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

#define ENBALE_FRAMEADMISSION 1

int sv_adapt_gmm()
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

	// AUDLOG( "ubm GMM as:\n" );
	// gmm_show( hUbm );

	entry   *pEntry    = NULL;
	dir     *pDir      = NULL;
	entry   *pSubEntry = NULL;
	dir     *pSubDir   = NULL;

	AUD_Int32s j           = 0;
	AUD_Int32s frameStride = 0;

	pDir = openDir( (const char*)wavPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		if ( pEntry->type == FILE_TYPE_DIRECTORY && pEntry->name[0] != '.' ) // a visible folder
		{
			AUD_Int32s totalWinNum = 0;

			char   dirName[256] = { 0, };
			snprintf( dirName, 256, "%s/%s", wavPath, pEntry->name );

			pSubDir = openDir( (const char*)dirName );
			while ( ( pSubEntry = scanDir( pSubDir ) ) )
			{
				AUD_Int8s   wavFile[256] = { 0, };
				AUD_Summary fileSummary;
				AUD_Int32s  sampleNum = 0;

				snprintf( (char*)wavFile, 256, "%s/%s", dirName, pSubEntry->name );

				ret = parseWavFromFile( wavFile, &fileSummary );
				if ( ret < 0 )
				{
					continue;
				}
				AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

				// AUDLOG( "%s\n", wavFile );

				// request memeory for template
				sampleNum = fileSummary.dataChunkBytes / fileSummary.bytesPerSample;
				for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
				{
					;
				}
				j = j - MFCC_DELAY;

				totalWinNum += j;
			}
			closeDir( pSubDir );
			pSubDir   = NULL;
			pSubEntry = NULL;

			AUD_Matrix featureMatrix;
			featureMatrix.rows     = totalWinNum;
			featureMatrix.cols     = MFCC_FEATDIM;
			featureMatrix.dataType = AUD_DATATYPE_INT32S;
			ret = createMatrix( &featureMatrix );
			AUD_ASSERT( ret == 0 );

			AUD_Double trainDur    = 0.;
			AUD_Int32s currentRow  = 0;
			pSubDir = openDir( (const char*)dirName );
			while ( ( pSubEntry = scanDir( pSubDir ) ) )
			{
				AUD_Int8s   wavFile[256] = { 0, };
				AUD_Summary fileSummary;
				AUD_Int32s  sampleNum        = 0;
				void        *hMfccHandle     = NULL;

				snprintf( (char*)wavFile, 256, "%s/%s", dirName, pSubEntry->name );

				ret = parseWavFromFile( wavFile, &fileSummary );
				if ( ret < 0 )
				{
					continue;
				}
				AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

				// AUDLOG( "\t %s\n", wavFile );

				AUD_Int32s bufLen = fileSummary.dataChunkBytes;
				AUD_Int16s *pBuf  = (AUD_Int16s*)calloc( bufLen, 1 );
				AUD_ASSERT( pBuf );

				sampleNum = readWavFromFile( (AUD_Int8s*)wavFile, pBuf, bufLen );
				AUD_ASSERT( sampleNum > 0 );

				trainDur += (AUD_Double)sampleNum / SAMPLE_RATE;

				// pre-processing

				 // pre-emphasis
				sig_preemphasis( pBuf, pBuf, sampleNum );

#if ( ENBALE_FRAMEADMISSION )
				// NOTE: frame admission
				AUD_Int32s validBufLen = sampleNum / FRAME_STRIDE * FRAME_LEN * BYTES_PER_SAMPLE;
				AUD_Int16s *pValidBuf  = (AUD_Int16s*)calloc( validBufLen, 1 );
				AUD_ASSERT( pValidBuf );

				AUD_Int32s validSampleNum = 0;
				validSampleNum = sig_frameadmit( pBuf, sampleNum, pValidBuf );

				j           = validSampleNum / FRAME_LEN;
				frameStride = FRAME_LEN;

				AUD_ASSERT( (j - MFCC_DELAY) > 10 );
				AUDLOG( "total frame: %d, valid frame: %d\n", sampleNum / FRAME_STRIDE - 1, j );
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
				feature.featureMatrix.pInt32s  = featureMatrix.pInt32s + currentRow * feature.featureMatrix.cols;

				feature.featureNorm.len      = j - MFCC_DELAY;
				feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
				ret = createVector( &(feature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
				AUD_ASSERT( error == AUD_ERROR_NONE );

#if ( ENBALE_FRAMEADMISSION )
				error = mfcc16s32s_calc( hMfccHandle, pValidBuf, validSampleNum, &feature );
				AUD_ASSERT( error == AUD_ERROR_NONE );
#else
				error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
				AUD_ASSERT( error == AUD_ERROR_NONE );
#endif

				error = mfcc16s32s_deinit( &hMfccHandle );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				free( pBuf );
				pBuf   = NULL;
				bufLen = 0;

#if ( ENBALE_FRAMEADMISSION )
				free( pValidBuf );
				pValidBuf   = NULL;
				validBufLen = 0;
#endif

				ret = destroyVector( &(feature.featureNorm) );
				AUD_ASSERT( ret == 0 );

				currentRow += feature.featureMatrix.rows;
			}
			closeDir( pSubDir );
			pSubDir   = NULL;
			pSubEntry = NULL;

			AUDLOG( "speaker[%s] total training material time: %.3f s\n", pEntry->name, trainDur );

			void      *hAdaptedGmm        = NULL;
			AUD_Int8s adaptedGmmName[256] = { 0, };
			snprintf( (char*)adaptedGmmName, 256, "%s", pEntry->name );
			error = gmm_clone( &hAdaptedGmm, hUbm, adaptedGmmName );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			AUD_Matrix trainMatrix;
			trainMatrix.rows     = currentRow;
			trainMatrix.cols     = MFCC_FEATDIM;
			trainMatrix.dataType = AUD_DATATYPE_INT32S;
			trainMatrix.pInt32s  = featureMatrix.pInt32s;

			// adapt GMM
			error = gmm_adapt( hAdaptedGmm, &trainMatrix );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			gmm_show( hAdaptedGmm );

			// export gmm
			char modelFile[256] = { 0 };
			snprintf( modelFile, 256, "%s/%s.gmm", SV_SPEAKER_GMMMODEL_DIR, adaptedGmmName );
			AUDLOG( "Export GMM Model to: %s\n", modelFile );
			FILE *fpGmm = fopen( modelFile, "wb" );
			AUD_ASSERT( fpGmm );

			error = gmm_export( hAdaptedGmm, fpGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			AUDLOG( "Export GMM Model File Done\n" );

			fclose( fpGmm );
			fpGmm = NULL;

			ret = destroyMatrix( &featureMatrix );
			AUD_ASSERT( ret == 0 );

			error = gmm_free( &hAdaptedGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
		}
	}
	closeDir( pDir );
	pDir   = NULL;
	pEntry = NULL;

	error = gmm_free( &hUbm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "SV GMM model adapt done\n" );

	return 0;
}


