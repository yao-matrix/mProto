#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"

#include "math/mmath.h"
#include "math/fft.h"


AUD_Int32s sign( AUD_Int16s x )
{
	if ( x > 0 )
	{
		return 1;
	}
	else if ( x < 0 )
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

AUD_Error zcr_calc( AUD_Int16s *pInBuffer, AUD_Int32s sampleNum, AUD_Int32s *pOutZCR )
{
	AUD_ASSERT( pInBuffer && pOutZCR && (sampleNum > 0) );

	AUD_Int32s i, sum;

	sum = 0;
	for ( i = 1; i < sampleNum; i++ )
	{
		sum += abs( sign( pInBuffer[i] ) -  sign( pInBuffer[i - 1] ) );
	}

	*pOutZCR = sum / 2;

	return AUD_ERROR_NONE;
}

/* absolute valued energy */
AUD_Error eabs_calc( AUD_Int16s *pInBuffer, AUD_Int32s sampleNum, AUD_Int32s *pOutEAbs )
{
	AUD_ASSERT( pInBuffer && pOutEAbs && (sampleNum > 0) );

	AUD_Int32s i;
	AUD_Int64u sum;

	sum = 0;
	for ( i = 0; i < sampleNum; i++ )
	{
		sum += abs( pInBuffer[i] );
	}

	*pOutEAbs = (AUD_Int32s)( sum / sampleNum );

	return AUD_ERROR_NONE;
}
