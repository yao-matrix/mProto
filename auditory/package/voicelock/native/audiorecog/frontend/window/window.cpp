#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"

AUD_Error win16s_init( AUD_Window16s **phWin, AUD_WindowType type, AUD_Int32s len, AUD_Int32s Q )
{
    AUD_Int32s    mulFactor, i;
    AUD_Window16s *pState = NULL;

    pState = (AUD_Window16s*)malloc( sizeof(AUD_Window16s) );
    if ( pState == NULL )
    {
        *phWin = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    pState->h = (AUD_Int16s*)malloc( sizeof(AUD_Int16s) * len );
    if ( pState->h == NULL )
    {
        free( pState );
        pState = NULL;
        *phWin = NULL;
        return AUD_ERROR_OUTOFMEMORY;
    }

    pState->len  = len;
    pState->Q    = Q;
    pState->type = type;
    mulFactor    = (1 << Q) - 1;

    switch ( type )
    {
    case AUD_WIN_HANN:
        for ( i = 0; i < len; i++ )
        {
            pState->h[i] = round( mulFactor * 0.5 * ( 1 - cos( TPI * i / ( len - 1 ) ) ) );
        }
        break;

    case AUD_WIN_HAMM:
        for ( i = 0; i < len; i++ )
        {
            pState->h[i] = round( mulFactor * ( 0.54 - 0.46 * cos( TPI * i / ( len - 1 ) ) ) );
        }
        break;

    default:
        free( pState->h );
        free( pState );
        pState = NULL;
        *phWin = NULL;
        return AUD_ERROR_UNSUPPORT;
    }

    *phWin = pState;

    return AUD_ERROR_NONE;
}

AUD_Error win16s_calc( AUD_Window16s *hWin, AUD_Int16s *x, AUD_Int16s *y )
{
    AUD_Int32s i, tmp;

    for ( i = 0; i < hWin->len; i++ )
    {
        tmp = x[i] * hWin->h[i];
        y[i] = tmp >> hWin->Q;
    }

    return AUD_ERROR_NONE;
}

AUD_Error win16s_free( AUD_Window16s **phWin )
{
    AUD_Window16s *pState = *phWin;

    free( pState->h );
    free( pState );

    *phWin = NULL;

    return AUD_ERROR_NONE;
}

