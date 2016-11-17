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


AUD_Int32s test_denoise()
{
	char            inputPath[256]  = { 0, };
	AUD_Int32s      data, ret;

	setbuf( stdin, NULL );
	AUDLOG( "pls give the folder of wav streams need to de-noise:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry       *pEntry         = NULL;
	dir         *pDir           = NULL;
	AUD_Int8s   noisyFile[256]  = { 0, };
	char        cleanFile[256]  = { 0, };
	AUD_Int32s  sampleNum;

	AUD_Int32s  bufLen          = 0;
	AUD_Int16s  *pOriBuf        = NULL;
	AUD_Int16s  *pCleanBuf      = NULL;

	pDir = openDir( (const char*)inputPath );
	if ( pDir == NULL )
	{
		AUDLOG( "cannot open folder: %s\n", inputPath );
		return -1;
	}

	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Summary fileSummary;

		snprintf( (char*)noisyFile, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( noisyFile, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );

		bufLen  = fileSummary.dataChunkBytes;
		pOriBuf = (AUD_Int16s*)calloc( bufLen, 1 );
		AUD_ASSERT( pOriBuf );

		sampleNum = readWavFromFile( noisyFile, pOriBuf, bufLen );
		if ( sampleNum < 0 )
		{
			continue;
		}
		AUDLOG( "stream[%s] sample number[%d]\n", noisyFile, sampleNum );

		pCleanBuf = (AUD_Int16s*)calloc( sampleNum * BYTES_PER_SAMPLE, 1 );
		AUD_ASSERT( pCleanBuf );

		snprintf( cleanFile, 256, "./data/clean/%s", pEntry->name );

		// denoise
#if defined( IPP )
		unsigned long long cStart = mips_gettick();
		long long          tStart = time_gettime();
#endif

		AUD_Int32s cleanLen = denoise_wiener( pOriBuf, pCleanBuf, sampleNum );

#if defined( IPP )
		unsigned long long cEnd   = mips_gettick();
		long long          tEnd   = time_gettime();
		long long cWork;
		if ( cStart < cEnd )
		{
			cWork = (long long)(cEnd - cStart);
		}
		else
		{
			cWork = (long long)(MAX_INT64S - cEnd + cStart);
		}
		long long tWork = tEnd - tStart;
		AUDLOG( "MCPS: [%.3f], time consumption: [%.3f s]\n", cWork / 1000000., tWork / 1000000. );
#endif

		writeWavToFile( (AUD_Int8s*)cleanFile, pCleanBuf, cleanLen );

		free( pOriBuf );
		pOriBuf   = NULL;
		free( pCleanBuf );
		pCleanBuf = NULL;
	}
	closeDir( pDir );
	pDir = NULL;

	AUDLOG( "denoise test finished\n" );

	return 0;
}
