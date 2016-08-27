#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"
#include "math/mmath.h"

AUD_Int32s test_mfcc_individual()
{
	char            inputPath[256]   = { 0, };
	AUD_Int32s      data;

	AUD_Error       error = AUD_ERROR_NONE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry   *pEntry        = NULL;
	dir     *pDir          = NULL;

	AUD_Int32s      sampleNum;

	AUD_Int32s      bufLen        = 0;
	AUD_Int16s      *pBuf         = NULL;

	AUD_Int32s      frameStride   = FRAME_STRIDE;

	AUD_Int32s      j;
	AUD_Int32s      ret;

	pDir = openDir( (const char*)inputPath );
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
		AUDLOG( "input stream: %s\n", inputStream );

		bufLen = fileSummary.dataChunkBytes;
		pBuf   = (AUD_Int16s*)calloc( bufLen + 100, 1 );
		AUD_ASSERT( pBuf );

		sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );

		for ( j = 0; j * frameStride + FRAME_LEN <= sampleNum; j++ )
		{
			;
		}

		AUD_Feature inFeature;
		inFeature.featureMatrix.rows     = j;
		inFeature.featureMatrix.cols     = MFCC_FEATDIM;
		inFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		inFeature.featureNorm.len      = j;
		inFeature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, frameStride, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

        // export feature matrix
		AUD_Int8s   mfccFile[256] = { 0, };
        AUD_Int8s   fileName[256] = {0, };

        int i = 0;
        while ( pEntry->name[i] != '.')
        {
            fileName[i] = pEntry->name[i];
            i++;
        }
		snprintf( (char*)mfccFile, 256, "%s/%s.mfcc", (const char*)inputPath, fileName );
		ret = printMatrixToFile( &(inFeature.featureMatrix), mfccFile );
		AUD_ASSERT( ret == 0 );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		bufLen  = 0;
		free( pBuf );
		pBuf = NULL;
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "Test MFCC individual finished\n" );

	return 0;
}
