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


AUD_Int32s test_pitch_tracking()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data;

	setbuf( stdin, NULL );
	AUDLOG( "pls give test wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "test stream path is: %s\n", inputPath );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry      *pEntry         = NULL;
	dir        *pDir           = NULL;
	AUD_Int8s  fileName[256]   = { 0, };
	AUD_Int32s sampleNum;

	AUD_Int32s bufLen          = 0;
	AUD_Int16s *pBuf           = NULL;


	FILE *fPitch = fopen( "./pitch.log", "wb" );
	AUD_ASSERT( fPitch );

	pDir = openDir( (const char*)inputPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		bufLen = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
		pBuf   = (AUD_Int16s*)malloc( bufLen );
		AUD_ASSERT( pBuf );

		snprintf( (char*)fileName, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		sampleNum = readWavFromFile( fileName, pBuf, bufLen );
		if ( sampleNum < 0 )
		{
			continue;
		}
		AUDLOG( "stream[%s] sample number[%d]\n", fileName, sampleNum );

		sig_preemphasis( pBuf, pBuf, sampleNum );

		// pitch tracking
		AUD_Int32s pitch;
		pitch_track( pBuf, sampleNum, &pitch );
		fprintf( fPitch, "%d, ", pitch );
		AUDLOG( "%d, \n", pitch );

		free( pBuf );
		pBuf = NULL;
	}

	fclose( fPitch );
	fPitch = NULL;

	AUDLOG( "PITCH-TRACKING finished\n" );

	return 0;
}
