#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

/* pitch tracking */
AUD_Double double_dot( AUD_Int16s *x, AUD_Int16s *y, AUD_Int32s len )
{
	AUD_Double sum = 0.;
	AUD_Int32s i;

	for ( i = 0; i < len; i++ )
	{
		sum += (AUD_Double)x[i] * y[i];
	}

	return sum;
}

AUD_Double double_amd( AUD_Int16s *x, AUD_Int16s *y, AUD_Int32s len )
{
	AUD_Double sum = 0.;
	AUD_Int32s i;

	for ( i = 0; i < len; i++ )
	{
		sum += (AUD_Double)abs( x[i] - y[i] );
	}
	return sum;
}

AUD_Error pitch_track( AUD_Int16s *pInBuf, AUD_Int32s len, AUD_Int32s *pPitch )
{
#define PITCH_FRAME_SIZE   512
#define PITCH_FRAME_STRIDE 256
#define HEAD_TAIL_SKIP     50

	AUD_Int32s i = 0, j = 0;
	AUD_Double acf[PITCH_FRAME_STRIDE]                              = { 0., };
	AUD_Double amdf[PITCH_FRAME_STRIDE]                             = { 0., };
	AUD_Double acfOverAmdf[PITCH_FRAME_STRIDE - HEAD_TAIL_SKIP * 2] = { 0., };
	AUD_Double maxV = -100.;
	AUD_Int32s maxI = -1;

	// check frame rate per audio
	AUD_Int32s frameNum = 0;
	for ( i = 0; i * PITCH_FRAME_SIZE + PITCH_FRAME_SIZE < len; i++ )
	{
		frameNum++;
	}

	for ( i = 0; i < frameNum; i++ )
	{
		for ( j = 0; j < PITCH_FRAME_STRIDE; j++ )
		{
			acf[j]  += double_dot( pInBuf + i * PITCH_FRAME_SIZE, pInBuf + i * PITCH_FRAME_SIZE + j, PITCH_FRAME_STRIDE ) / frameNum;
			amdf[j] += double_amd( pInBuf + i * PITCH_FRAME_SIZE, pInBuf + i * PITCH_FRAME_SIZE + j, PITCH_FRAME_STRIDE ) / frameNum;
		}
	}

	for ( i = 0; i < PITCH_FRAME_STRIDE - HEAD_TAIL_SKIP * 2; i++ )
	{
		acfOverAmdf[i] = acf[HEAD_TAIL_SKIP + i] / amdf[HEAD_TAIL_SKIP + i];

		if ( maxV < acfOverAmdf[i] )
		{
			maxV = acfOverAmdf[i];
			maxI = i;
		}
	}

	*pPitch = (AUD_Int32s)round( SAMPLE_RATE / ( (AUD_Double)( maxI + HEAD_TAIL_SKIP ) ) );

	return AUD_ERROR_NONE;
}
