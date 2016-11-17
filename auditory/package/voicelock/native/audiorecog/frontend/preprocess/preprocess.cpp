#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"
#include "AudioConfig.h"

/* signal normalization */
void sig_normalize( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len )
{
    AUD_Int32s i, v;
    AUD_Double mean = 0;

    for ( i = 0; i < len; i++ )
    {
        mean += pInBuf[i];
    }

    mean /= len;

    for ( i = 0; i < len; i++ )
    {
        v = (AUD_Int32s)round( pInBuf[i] - mean );

        v = SATURATE_16s( v );

        pOutBuf[i] = (AUD_Int16s)v;
    }


#if 1
    AUD_Double x = 0;
    AUD_Double tmp;
    for ( i = 0; i < len; i++ )
    {
        x += (AUD_Double)pOutBuf[i] * pOutBuf[i];
    }
    x /= len;
    // x  = sqrt( x );

    if ( x < 1024 )
    {
        for ( i = 0; i < len; i++ )
        {
            // tmp = pOutBuf[i] / x * 8192;
            tmp = pOutBuf[i] * 256;

            tmp = SATURATE_16s( tmp );

            pOutBuf[i] = (AUD_Int16s)tmp;
        }
    }
#endif

#if 0
    AUD_Int16s maxMag = 0, mag = 0;
    for ( i = 0; i < len; i++ )
    {
        mag = abs( pOutBuf[i] );
        if ( maxMag < mag )
        {
            maxMag = mag;
        }
    }

    for ( i = 0; i < len; i++ )
    {
        pOutBuf[i] = (AUD_Int16s)( (AUD_Double)pOutBuf[i] / maxMag * 0.25 * 32768. );
    }
#endif

    return;
}

/* signal pre-emphasis */
void sig_preemphasis( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len )
{
    AUD_Int16s x0, x1;
    AUD_Double out;
    AUD_Int32s i;

    x1 = pInBuf[0];

    for ( i = 1; i < len; i++ )
    {
        x0  = pInBuf[i];
        out = x0 - PREEMPHASIS_FACTOR * x1;

        out = SATURATE_16s( out );

        pOutBuf[i] = (AUD_Int16s)out;

        x1 = x0;
    }

    pOutBuf[0] = (AUD_Int16s)( ( 1 - PREEMPHASIS_FACTOR ) * pInBuf[0] );

    return;
}

/* signal de-emphasis */
void sig_deemphasis( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len )
{
    AUD_Int16s y1;
    AUD_Double out;
    AUD_Int32s i;

    y1 = 0;

    for ( i = 0; i < len; i++ )
    {
        out = pInBuf[i] + PREEMPHASIS_FACTOR * y1;

        out = SATURATE_16s( out );

        pOutBuf[i] = (AUD_Int16s)out;

        y1 = pOutBuf[i];
    }

    return;
}

