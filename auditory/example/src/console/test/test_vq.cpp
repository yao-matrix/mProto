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

#define ENBALE_FRAMEADMISSION 0

#define MALE_VQ_FILE    "./vq/male.vq"
#define FEMALE_VQ_FILE  "./vq/female.vq"

static AUD_Int32s genderTable[11] = { 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0 }; // 1 means male, 0 means female

AUD_Int32s test_vq()
{
	AUD_Error  error          = AUD_ERROR_NONE;
	AUD_Int8s  inputPath[256] = { 0, };

	AUD_Int32s ret, data;
	AUD_Int32s i, j;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	// read VQ model from file
	AUD_Matrix maleVQMatrix;
	AUD_Matrix femaleVQMatrix;
	ret = readMatrixFromFile( &maleVQMatrix, (AUD_Int8s*)MALE_VQ_FILE );
	AUD_ASSERT( ret == 0 );

	ret = readMatrixFromFile( &femaleVQMatrix, (AUD_Int8s*)FEMALE_VQ_FILE );
	AUD_ASSERT( ret == 0 );

	int success = 0, fail = 0;

	dir   *pWavDir   = openDir( (const char*)inputPath );
	entry *pWavEntry = NULL;
	while ( ( pWavEntry = scanDir( pWavDir ) ) )
	{
		AUD_Int8s   inputStream[256] = { 0, };
		void        *hMfccHandle     = NULL;
		AUD_Summary fileSummary;

		snprintf( (char*)inputStream, 256, "%s/%s", (const char*)inputPath, pWavEntry->name );

		ret = parseWavFromFile( inputStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		AUD_Int32s bufLen = fileSummary.dataChunkBytes;
		AUD_Int16s *pBuf  = (AUD_Int16s*)calloc( bufLen + 100, 1 );
		AUD_ASSERT( pBuf );

		AUD_Int32s sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );

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

		// compare code book
		AUD_Double maleDistance = 0., femaleDistance = 0.;
		AUD_Double finalScore;
		for ( i = 0; i < feature.featureMatrix.rows; i++ )
		{
			maleDistance   += calc_nearestdist( &(feature.featureMatrix), i, &maleVQMatrix );
			femaleDistance += calc_nearestdist( &(feature.featureMatrix), i, &femaleVQMatrix );
		}
		maleDistance  /= feature.featureMatrix.rows;
		femaleDistance /= feature.featureMatrix.rows;
		finalScore = femaleDistance - maleDistance;

		char   *ptr    = strchr( pWavEntry->name, (int)'-' );
		char   num[10] = "\0";
		size_t len     = 0;
		ptr--;
		if ( ptr )
		{
			while ( ptr > (char*)pWavEntry->name )
			{
				len++;
				ptr--;
			}
			len++;
		}
		else
		{
			len = strlen( (char*)pWavEntry->name );
		}
		AUD_ASSERT( len > 0 );

		snprintf( num, len + 1, "%s", pWavEntry->name );
		int speaker = atoi( num );
		// AUDLOG( "speaker: %d\n", speaker );

		int detectedGender = ( finalScore > 0 ? 1 : 0 );
		if ( detectedGender == genderTable[speaker - 1] )
		{
			// AUDLOG( "\tSuccess! stream[%s]: male distance: %.2f, female distance: %.2f, final score: %.2f\n", inputStream, maleDistance, femaleDistance, finalScore );
			success++;
		}
		else
		{
			AUDLOG( "\tFail! stream[%s]: male distance: %.2f, female distance: %.2f, final score: %.2f\n", inputStream, maleDistance, femaleDistance, finalScore );
			fail++;
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

#if ( ENBALE_FRAMEADMISSION )
		free( pValidBuf );
		pValidBuf   = NULL;
		validBufLen = 0;
#endif
	}
	closeDir( pWavDir );
	pWavDir = NULL;

	ret = destroyMatrix( &maleVQMatrix );
	AUD_ASSERT( ret == 0 );
	ret = destroyMatrix( &femaleVQMatrix );
	AUD_ASSERT( ret == 0 );

	AUDLOG( "success: %d, fail: %d\n", success, fail );
	AUDLOG( "VQ Benchmark finished\n" );

	return 0;
}
