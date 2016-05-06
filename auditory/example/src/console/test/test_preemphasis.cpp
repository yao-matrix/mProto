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


AUD_Int32s test_preemphasis()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data, ret;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry           *pEntry        = NULL;
	dir             *pDir          = NULL;
	AUD_Int8s       origFile[256]  = { 0, };
	char            preedFile[256] = { 0, };
	AUD_Int32s      sampleNum;

	AUD_Int32s      origBufLen     = 0;
	AUD_Int16s      *pOriBuf       = NULL;

	pDir = openDir( (const char*)inputPath );
	if ( pDir == NULL )
	{
		AUDLOG( "cannot open folder: %s\n", inputPath );
		return -1;
	}

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

		// pre-emphasis
		sig_preemphasis( pOriBuf, pOriBuf, sampleNum );

		snprintf( preedFile, 256, "./data/preed/%s", pEntry->name );
		ret = writeWavToFile( (AUD_Int8s*)preedFile, pOriBuf, sampleNum );
		AUD_ASSERT( ret == 0 );

		free( pOriBuf );
		pOriBuf    = NULL;
		origBufLen = 0;
	}
	closeDir( pDir );
	pDir = NULL;


	AUDLOG( "pre-emphasis test finished\n" );

	return 0;
}
