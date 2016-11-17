#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <math.h>


#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "misc/misc.h"
#include "io/file/io.h"

#define VQ_MIN_THETA    -1.
#define VQ_MAX_ITER     25
#define VQ_ATTEMPTS     10

#define ENBALE_FRAMEADMISSION       1
#define ENABLE_FEATURENORMALIZATION 0

int kmeans_vq( )
{
	AUD_Int8s  filePath[256]  = { 0, };
	AUD_Int8s  vqFile[256]    = { 0, };
	AUD_Error  error          = AUD_ERROR_NONE;
	AUD_Tick   t;

	AUD_Int32s    data, ret;
	AUD_Int32s    j;
	AUD_Int32s    totalWinNum;
	AUD_Int32s    sampleNum     = 0;
	AUD_Int32s    bufLen        = 0;
	AUD_Int16s    *pBuf         = NULL;

	AUD_Int32s    vqOrder      = -1;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give training wav stream's folder path:\n" );
	filePath[0] = '\0';
	data = scanf( "%s", filePath );
	AUDLOG( "training folder path is: %s\n", filePath );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give generated VQ file:\n" );
	data = scanf( "%s", vqFile );
	AUDLOG( "generated VQ file will be: %s\n", vqFile );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give VQ order to be trained:\n" );
	data = scanf( "%d", &vqOrder );
	AUDLOG( "VQ order to be trained is: %d\n", vqOrder );

	entry *pEntry = NULL;
	dir   *pDir   = NULL;

	// scan directory & collect stream info
	totalWinNum = 0;
	pDir = openDir( (const char*)filePath );
	if ( pDir == NULL )
	{
		AUDLOG( "cannot open folder: %s\n", filePath );
		return -1;
	}

	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Int8s   inputStream[256] = { 0, };
		AUD_Summary fileSummary;

		snprintf( (char*)inputStream, 256, "%s/%s", (const char*)filePath, pEntry->name );

		// parse each file
		ret = parseWavFromFile( inputStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		AUD_Int32s sampleLen = fileSummary.dataChunkBytes / fileSummary.bytesPerSample;
		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleLen; j++ )
		{
			;
		}
		j = j - MFCC_DELAY;

		totalWinNum += j;
	}
	closeDir( pDir );
	pDir = NULL;

	// apply for feature matrix memory
	AUD_Matrix featureMatrix;
	featureMatrix.rows     = totalWinNum;
	featureMatrix.cols     = MFCC_FEATDIM;
	featureMatrix.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &featureMatrix );
	AUD_ASSERT( ret == 0 );

	// loop training directory & extract MFCC
	AUD_Int32s streamID    = 0;
	AUD_Int32s currentRow  = 0;
	AUD_Int32s frameStride = 0;
	pDir = openDir( (const char*)filePath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		void        *hMfccHandle  = NULL;
		AUD_Int8s   fileName[256] = { 0, };
		AUD_Summary fileSummary;

		snprintf( (char*)fileName, 256, "%s/%s", (const char*)filePath, pEntry->name );

		// parse wav file
		ret = parseWavFromFile( fileName, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		bufLen = fileSummary.dataChunkBytes;
		pBuf   = (AUD_Int16s*)calloc( bufLen + 100, 1 );
		AUD_ASSERT( pBuf );

		// read stream from file
		sampleNum = readWavFromFile( (AUD_Int8s*)fileName, pBuf, bufLen );
		if ( sampleNum <= 0 )
		{
			continue;
		}

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );

#if ( ENBALE_FRAMEADMISSION )
		 // NOTE: frame admission
		AUD_Int32s validBufLen = sampleNum / FRAME_STRIDE * FRAME_LEN * BYTES_PER_SAMPLE;
		AUD_Int16s *pValidBuf  = (AUD_Int16s*)calloc( validBufLen, 1 );
		AUD_ASSERT( pValidBuf );

		AUDLOG( "input stream: %s, ", fileName );
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

		// feature matrix
		AUD_Feature feature;
		feature.featureMatrix.rows     = j - MFCC_DELAY;
		feature.featureMatrix.cols     = MFCC_FEATDIM;
		feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		feature.featureMatrix.pInt32s  = featureMatrix.pInt32s + currentRow * feature.featureMatrix.cols;

		feature.featureNorm.len      = j - MFCC_DELAY;
		feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc MFCC feature
#if ( ENBALE_FRAMEADMISSION )
		error = mfcc16s32s_calc( hMfccHandle, pValidBuf, validSampleNum, &feature );
		AUD_ASSERT( error == AUD_ERROR_NONE );
#else
		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
		AUD_ASSERT( error == AUD_ERROR_NONE );
#endif

#if ENABLE_FEATURENORMALIZATION
		// NOTE: normalize feature
		int i, k;
		for ( i = 0; i < feature.featureMatrix.rows; i++ )
		{
			AUD_Int32s *pFeatureVec = feature.featureMatrix.pInt32s + i * feature.featureMatrix.cols;
			AUD_Int32s segNum  = (featureMatrix.cols + MFCC_ORDER - 1) / MFCC_ORDER;
			AUD_Double segNorm = 0.;
			for ( j = 0; j < segNum; j++ )
			{
				segNorm = 0.;
				for ( k = j * MFCC_ORDER; k < AUD_MIN( (j + 1) * MFCC_ORDER, featureMatrix.cols ); k++ )
				{
					segNorm += pFeatureVec[k] * pFeatureVec[k];
				}
				segNorm = sqrt( segNorm );

				for ( k = j * MFCC_ORDER; k < AUD_MIN( (j + 1) * MFCC_ORDER, featureMatrix.cols ); k++ )
				{
					pFeatureVec[k] = (AUD_Int32s)round( pFeatureVec[k] / segNorm * 65536. );
				}
			}
		}
#endif

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		AUDLOG( "Extract MFCC of stream [%d] Done\n", streamID );
		currentRow += feature.featureMatrix.rows;
		streamID++;

		free( pBuf );
		pBuf   = NULL;
		bufLen = 0;

#if ( ENBALE_FRAMEADMISSION )
		free( pValidBuf );
		pValidBuf   = NULL;
		validBufLen = 0;
#endif
	}
	closeDir( pDir );
	pDir = NULL;

	// training matrix
	AUD_Matrix trainMatrix;
	trainMatrix.rows      = currentRow;
	trainMatrix.cols      = MFCC_FEATDIM;
	trainMatrix.dataType  = AUD_DATATYPE_INT32S;
	trainMatrix.pInt32s   = featureMatrix.pInt32s;

	AUD_Matrix labelMatrix;
	labelMatrix.rows          = currentRow;
	labelMatrix.cols          = 1;
	labelMatrix.dataType      = AUD_DATATYPE_INT32S;
	ret = createMatrix( &labelMatrix );
	AUD_ASSERT( ret == 0 );

	AUD_Matrix centroidMatrix;
	centroidMatrix.rows       = vqOrder;
	centroidMatrix.cols       = MFCC_FEATDIM;
	centroidMatrix.dataType   = AUD_DATATYPE_INT32S;
	ret = createMatrix( &centroidMatrix );
	AUD_ASSERT( ret == 0 );

	// train VQ
	AUDLOG( "\nVQ training Start\n" );
	t = time_gettime();
	error = kmeans_cluster( &trainMatrix, vqOrder, VQ_MAX_ITER, VQ_MIN_THETA, VQ_ATTEMPTS, &labelMatrix, &centroidMatrix );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	t = time_gettime() - t;
	AUDLOG( "VQ training Done\n" );

	// export VQ file
	AUDLOG( "VQ file is\n" );
	showMatrix( &centroidMatrix );
	AUDLOG( "\n" );

	AUDLOG( "Export VQ Model File[%s] Start\n", vqFile );
	writeMatrixToFile( &centroidMatrix, (AUD_Int8s*)vqFile );
	AUDLOG( "Export VQ Model File Done\n" );

	ret = destroyMatrix( &featureMatrix );
	AUD_ASSERT( ret == 0 );
	ret = destroyMatrix( &labelMatrix );
	AUD_ASSERT( ret == 0 );
	ret = destroyMatrix( &centroidMatrix );
	AUD_ASSERT( ret == 0 );

	AUDLOG( "Finish VQ training, Time Elapsed: %.2f s\n\n", t / 1000000.0 );

	return 0;
}
