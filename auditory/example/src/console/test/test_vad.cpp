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

#include "benchmark.h"


AUD_Int32s test_vad()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data, ret, i;
	AUD_Int32s      missNum = 0;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry       *pEntry          = NULL;
	dir         *pDir            = NULL;
	AUD_Int8s   origFile[256]    = { 0, };
	char        vadedDir[256]    = { 0, };
	char        vadedFile[256]   = { 0, };
	char        denoiseDir[256]  = { 0, };
	char        denoiseFile[256] = { 0, };
	AUD_Int32s  sampleNum;

	AUD_Int32s  origBufLen      = 0;
	AUD_Int16s  *pOriBuf        = NULL;
	AUD_Int32s  speechBufLen    = 0;
	AUD_Int16s  *pSpeechBuf     = NULL;
	AUD_Int16s  *pSpeechFlag    = NULL;
	AUD_Int16s  *pFrameBuf      = NULL;
	pFrameBuf = (AUD_Int16s*)calloc( FRAME_LEN * BYTES_PER_SAMPLE, 1 );
	AUD_ASSERT( pFrameBuf );

	void        *hVadHandle     = NULL;

	pDir = openDir( (const char*)inputPath );
	if ( pDir == NULL )
	{
		AUDLOG( "cannot open folder: %s\n", inputPath );
		return -1;
	}

	int  slash = '/';
	char *ptr = strrchr( inputPath, slash );
	ptr++;

	snprintf( vadedDir, 256, "./data/vaded/%s", ptr );
	AUDLOG( "vaded directory: %s\n", vadedDir );
	ret = mkDir( vadedDir, 0777 );
	AUD_ASSERT( ret == 0 || ret == -1 );

	snprintf( denoiseDir, 256, "./data/clean/%s", ptr );
	AUDLOG( "clean directory: %s\n", denoiseDir );
	ret = mkDir( denoiseDir, 0777 );
	AUD_ASSERT( ret == 0 || ret == -1 );

	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Summary fileSummary;

		snprintf( (char*)origFile, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( origFile, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		origBufLen = fileSummary.dataChunkBytes;
		pOriBuf    = (AUD_Int16s*)calloc( origBufLen, 1 );
		AUD_ASSERT( pOriBuf );

		sampleNum = readWavFromFile( origFile, pOriBuf, origBufLen );
		if ( sampleNum < 0 )
		{
			continue;
		}
		AUDLOG( "stream[%s] sample number[%d]\n", origFile, sampleNum );

		speechBufLen = sampleNum * BYTES_PER_SAMPLE * 2;
		pSpeechBuf   = (AUD_Int16s*)calloc( speechBufLen, 1 );
		AUD_ASSERT( pSpeechBuf );
		pSpeechFlag  = (AUD_Int16s*)calloc( ( sampleNum / VAD_FRAME_LEN ) + 1, sizeof(AUD_Int16s) );
		AUD_ASSERT( pSpeechFlag );

		// vad
		ret = sig_vadinit( &hVadHandle, pSpeechBuf, pSpeechFlag, speechBufLen, 6, 6 );
		AUD_ASSERT( ret == 0 );

		AUD_Int32s start = -1;
		snprintf( vadedFile, 256, "%s/%s", vadedDir, pEntry->name );
		for ( i = 0; i * VAD_FRAME_LEN + FRAME_LEN <= sampleNum; i++ )
		{
			ret = sig_vadupdate( hVadHandle, pOriBuf + i * VAD_FRAME_LEN, &start );
			if ( ret == 0 )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		sig_vaddeinit( &hVadHandle );

		if ( ret != 0 )
		{
			AUDLOG( "found speech in stream\n" );
			writeWavToFile( (AUD_Int8s*)vadedFile, pSpeechBuf + start, ret );

			FILE *fp = fopen( "./test_flag.txt", "ab+" );
			fprintf( fp, "%s\n", pEntry->name );
			for ( i = start / VAD_FRAME_LEN; i < ( start + ret ) / VAD_FRAME_LEN; i++ )
			{
				fprintf( fp, "%d ", pSpeechFlag[i] );
			}
			fclose( fp );
			fp = NULL;

			// denoise
			AUD_Int16s* pCleanBuf = (AUD_Int16s*)calloc( (ret + start) * BYTES_PER_SAMPLE, 1 );
			AUD_ASSERT( pCleanBuf );

			AUD_Int32s cleanLen = denoise_wiener( pSpeechBuf, pCleanBuf, (ret + start) );
			AUD_ASSERT( cleanLen > 0 );

			snprintf( denoiseFile, 255, "%s/%s", denoiseDir, pEntry->name );
			ret = writeWavToFile( (AUD_Int8s*)denoiseFile, pCleanBuf, cleanLen );
			AUD_ASSERT( ret == 0 );

			free( pCleanBuf );
			pCleanBuf  = NULL;
		}
		else
		{
			AUDLOG( "cannot find speech in stream\n" );

			FILE *fp = fopen( "./missed_files.log", "ab+" );
			fprintf( fp, "%s\n", pEntry->name );
			fclose( fp );
			fp = NULL;

			missNum++;
		}

		free( pOriBuf );
		pOriBuf    = NULL;
		free( pSpeechBuf );
		pSpeechBuf = NULL;
		free( pSpeechFlag );
		pSpeechFlag = NULL;
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "total missed number: %d\n", missNum );

	free( pFrameBuf );
	pFrameBuf = NULL;

	AUDLOG( "VAD test finished\n" );

	return 0;
}
