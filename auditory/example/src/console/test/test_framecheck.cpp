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

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"

#include "math/mmath.h"

#include "benchmark.h"


AUD_Int32s test_framecheck()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data, ret, i;

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
	AUD_Int32s  sampleNum;

    AUD_Int16s  *pOriBuf        = NULL;
	AUD_Int32s  origBufLen      = 0;
    AUD_Int32s  speechLen       = 0;
    AUD_Int16s  *pSpeechBuf     = NULL;

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

	snprintf( vadedDir, 256, "./data/framecheck/%s", ptr );
	AUDLOG( "vaded directory: %s\n", vadedDir );
	ret = mkDir( vadedDir, 0777 );
	AUD_ASSERT( ret == 0 || ret == -1 );

    ret = sig_vadinit( &hVadHandle, NULL, NULL, 0, -1, 6 );
    AUD_ASSERT( ret == 0 );

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

        pSpeechBuf = (AUD_Int16s*)calloc( origBufLen, 1 );
        AUD_ASSERT( pSpeechBuf );

		snprintf( vadedFile, 256, "%s/%s", vadedDir, pEntry->name );

        AUD_Int16s *pSpeech = pSpeechBuf;
        speechLen = 0;
		for ( i = 0; i * VAD_FRAME_LEN + FRAME_LEN <= sampleNum; i++ )
		{
            // frame admission
            AUD_Int16s flag;
            ret = sig_framecheck( hVadHandle, pOriBuf + i * VAD_FRAME_LEN, &flag );
            if ( ret != 1 )
            {
                continue;
            }
            else
            {
                memmove( pSpeech, pOriBuf + i * VAD_FRAME_LEN, VAD_FRAME_LEN * BYTES_PER_SAMPLE );
                pSpeech += VAD_FRAME_LEN;
                speechLen += VAD_FRAME_LEN;
            }
		}

        AUDLOG( "frame checked file[%s], sample number: %d\n", vadedFile, speechLen );
        ret = writeWavToFile( (AUD_Int8s*)vadedFile, pSpeechBuf, speechLen );
        AUD_ASSERT(ret == 0);

		free( pOriBuf );
		pOriBuf = NULL;

        free( pSpeechBuf );
        pSpeechBuf = NULL;
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "frame check test finished\n" );

	return 0;
}
