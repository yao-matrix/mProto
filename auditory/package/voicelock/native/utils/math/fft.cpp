#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "AudioDef.h"
#include "AudioUtil.h"

#include "mmath.h"
#include "fft.h"

/* Decimation-In-Frequency FFT */
static void fftDif( Fft_16s *pFft, short *sh )
{
    int    stage, group, but, Nstage, Ngroup, Nbut, Gincr, Bincr;
    int    wind, wincr;
    double acc2r, acc2i, wr, wi;

    Nstage = pFft->numbits;
    Gincr  = pFft->len;
    wincr  = 1;
    *sh    = 0;
    for ( stage = 0; stage < Nstage; stage++ )
    {
        Ngroup = 1 << stage;
        Nbut   = Gincr >> 1;
        Bincr  = Nbut;

        for ( group = 0; group < Ngroup * Gincr; group += Gincr )
        {
            wind = 0;
            for ( but = group; but < group + Nbut; but++ )
            {
                // printf( "i = %d, l = %d\n", but, but + Bincr );
                wr = pFft->wr[wind];
                wi = pFft->wi[wind];
                wind += wincr;
                // printf( "wind = %d, wr = %d, wi = %d\n", wind, wr, wi );

                acc2r = pFft->xr[but] - pFft->xr[but + Bincr];
                acc2i = pFft->xi[but] - pFft->xi[but + Bincr];

                pFft->xr[but] = pFft->xr[but] + pFft->xr[but + Bincr];
                pFft->xi[but] = pFft->xi[but] + pFft->xi[but + Bincr];

                pFft->xr[but + Bincr] = acc2r * wr - acc2i * wi;
                pFft->xi[but + Bincr] = acc2i * wr + acc2r * wi;
                // printf( "%d + j%d, %d + j%d\n", xr[but], xi[but], xr[but + Bincr], xi[but + Bincr] );
            }
        }
        Gincr = Gincr >> 1;
        wincr = wincr << 1;
    }

    return;
}

static void fftShuffle( Fft_16s *pFft )
{
    double rtmp, itmp;
    int    i, bitrev_i;
    int    j;

    for ( i = 0; i < pFft->len; i++ )
    {
        bitrev_i = 0;
        for ( j = 0; j < pFft->numbits; j++ )
        {
            bitrev_i = bitrev_i | ( ( ((1 << j) & i) >> j ) << ( pFft->numbits - 1 - j ) );
        }

        // AUDLOG( "i = %d, bitrev_i = %d\n", i, bitrev_i );
        if ( i < bitrev_i )
        {
            rtmp               = pFft->xr[i];
            pFft->xr[i]        = pFft->xr[bitrev_i];
            pFft->xr[bitrev_i] = rtmp;
            itmp               = pFft->xi[i];
            pFft->xi[i]        = pFft->xi[bitrev_i];
            pFft->xi[bitrev_i] = itmp;
        }
    }

    return;
}

void fft_init( Fft_16s **phFft, int len, int Q )
{
    int i;

    *phFft = (Fft_16s*)malloc( sizeof(Fft_16s) );
    if ( *phFft == NULL )
        goto fft_init_fail1;

    (*phFft)->len = len;
    (*phFft)->Q   = Q;
    (*phFft)->wr  = (double*)calloc( sizeof(double) * len / 2, 1 );
    if ( (*phFft)->wr == NULL)
        goto fft_init_fail2;
    (*phFft)->wi  = (double*)calloc( sizeof(double) * len / 2, 1 );
    if ( (*phFft)->wi == NULL)
        goto fft_init_fail3;
    (*phFft)->xr  = (double*)calloc( sizeof(double) * len, 1 );
    if ( (*phFft)->xr == NULL)
        goto fft_init_fail4;
    (*phFft)->xi  = (double*)calloc( sizeof(double) * len, 1 );
    if ( (*phFft)->xi == NULL)
        goto fft_init_fail5;

    for ( i = 0; i < len / 2; i++ )
    {
        (*phFft)->wr[i] = cos( TPI * i / len );
        (*phFft)->wi[i] = -sin( TPI * i / len );
        // printf( "%d + j%d\n", (*phFft)->wr[i], (*phFft)->wi[i] );
    }

    for ( i = 0; i < 16; i++ )
    {
        if ( ( (*phFft)->len >> i ) == 1 )
        {
            (*phFft)->numbits = i;
            break;
        }
    }
    return;

fft_init_fail5:
    free( (*phFft)->xr );
fft_init_fail4:
    free( (*phFft)->wi );
fft_init_fail3:
    free( (*phFft)->wr );
fft_init_fail2:
    free( *phFft );
    *phFft = NULL;
fft_init_fail1:
    return;
}


void fft_mag( Fft_16s *pFft, short *xr, int inLen, int *Xabs )
{
    AUD_ASSERT( inLen <= pFft->len );

    int    i;
    double R, I;
    double X;
    short  sh;
    for ( i = 0; i < inLen; i++ )
    {
        pFft->xr[i] = (double)xr[i] / 32768.;
        pFft->xi[i] = 0;
    }
    if ( inLen < pFft->len )
    {
        for ( i = inLen; i < pFft->len; i++ )
        {
            pFft->xr[i] = 0;
            pFft->xi[i] = 0;
        }
    }

    fftDif( pFft, &sh );
    fftShuffle( pFft );

    double tmp;
    for ( i = 0; i < pFft->len; i++ )
    {
        R = pFft->xr[i];
        I = pFft->xi[i];
        X = (R * R) + (I * I);
        tmp = sqrt( X ) * 32768.;

        if ( tmp > MAX_INT32S )
        {
            Xabs[i] = MAX_INT32S;
        }
        else
        {
            Xabs[i] = (int)round( tmp );
        }
    }

#if 0
    for ( i = 0; i < pFft->len; i++ )
    {
        AUDLOG( "%d, ", Xabs[i] );
    }
    AUDLOG( "\n" );
#endif

    return;
}

void fft_calc( Fft_16s *pFft, short *xr, int inLen, int *Xre, int *Xim )
{
    AUD_ASSERT( inLen <= pFft->len );

    int    i;
    short  sh;

    for ( i = 0; i < inLen; i++ )
    {
        pFft->xr[i] = (double)xr[i] / 32768.;
        pFft->xi[i] = 0;
    }
    if ( inLen < pFft->len )
    {
        for ( i = inLen; i < pFft->len; i++ )
        {
            pFft->xr[i] = 0;
            pFft->xi[i] = 0;
        }
    }

    fftDif( pFft, &sh );
    fftShuffle( pFft );

#if 0
    AUDLOG( "fft: \n" );
    for ( i = 0; i < pFft->len; i++ )
    {
        AUDLOG( "%.2f + j%.2f, ", pFft->xr[i], pFft->xi[i] );
    }
    AUDLOG( "\n" );
#endif

    for ( i = 0; i < pFft->len; i++ )
    {
        Xre[i] = (int)round( pFft->xr[i] * 32768. );
        Xim[i] = (int)round( pFft->xi[i] * 32768. );
    }

    return;
}


void fft_free( Fft_16s **phFft )
{
    free( (*phFft)->wi );
    free( (*phFft)->wr );
    free( (*phFft)->xi );
    free( (*phFft)->xr );
    free( *phFft );

    *phFft = NULL;

    return;
}

// IFFT
void ifft_init( Ifft_16s **phIfft, int len, int Q )
{
    int i, k;
    int S;
    double *pWr = (*phIfft)->wr;
    double *pWi = (*phIfft)->wi;

    *phIfft = (Ifft_16s*)malloc( sizeof(Ifft_16s) );
    if ( *phIfft == NULL )
        goto ifft_init_fail_1;

    S = ( 1 << Q ) - 1;

    (*phIfft)->len = len;
    (*phIfft)->Q   = Q;
    (*phIfft)->wr  = (double*)calloc( sizeof(double) * len * len, 1 );
    if ( (*phIfft)->wr == NULL)
        goto ifft_init_fail_2;
    (*phIfft)->wi  = (double*)calloc( sizeof(double) * len * len, 1 );
    if ( (*phIfft)->wi == NULL )
        goto ifft_init_fail_3;
    (*phIfft)->Xr  = (int*)calloc( sizeof(int) * len, 1 );
    if ( (*phIfft)->Xr == NULL )
        goto ifft_init_fail_4;
    (*phIfft)->Xi  = (int*)calloc( sizeof(int) * len, 1 );
    if ( (*phIfft)->Xi == NULL )
        goto ifft_init_fail_5;

    for ( i = 0; i < len; i++ )
    {
        for ( k = 0; k < len; k++ )
        {
            *pWr = cos( TPI * i * k / (double)len );
            *pWi = sin( TPI * i * k / (double)len );
            pWr++;
            pWi++;
        }
    }

    return;

ifft_init_fail_5:
    free( (*phIfft)->Xr );
ifft_init_fail_4:
    free( (*phIfft)->wi );
ifft_init_fail_3:
    free( (*phIfft)->wr );
ifft_init_fail_2:
    free( *phIfft );
    *phIfft = NULL;
ifft_init_fail_1:
    return;
}


void ifft_real( Ifft_16s *pIfft, int *Xr, int *Xi, int inLen, short *xr )
{
    AUD_ASSERT( inLen == pIfft->len );

    int    i, k;
    double x;

    for ( i = 0; i < inLen; i++ )
    {
        pIfft->Xr[i] = Xr[i];
        pIfft->Xi[i] = Xi[i];
    }
    if ( inLen < pIfft->len )
    {
        for ( i = inLen; i < pIfft->len; i++ )
        {
            pIfft->Xr[i] = 0;
            pIfft->Xi[i] = 0;
        }
    }

    double *pWr = pIfft->wr;
    double *pWi = pIfft->wi;
    int ceil = 0, floor = 0;
    for ( i = 0; i < pIfft->len; i++ )
    {
        x = 0.;
        for ( k = 0; k < pIfft->len; k++ )
        {
            x = x + (*pWr) * pIfft->Xr[k] - (*pWi) * pIfft->Xi[k];
            pWr++;
            pWi++;
        }
        x /= pIfft->len;

        if ( x > MAX_INT16S )
        {
            ceil++;
            // AUDLOG( "%.2f, ", x );
            xr[i] = MAX_INT16S;
        }
        else if ( x < -MAX_INT16S )
        {
            floor++;
            // AUDLOG( "%.2f, ", x );
            xr[i] = -MAX_INT16S;
        }
        else
        {
            xr[i] = (short)round( x );
        }
    }
    // AUDLOG( "ceil: %d, floor: %d\n", ceil, floor );

    return;
}

void ifft_free( Ifft_16s **phIfft )
{
    free( (*phIfft)->wi );
    free( (*phIfft)->wr );
    free( (*phIfft)->Xi );
    free( (*phIfft)->Xr );

    free( *phIfft );

    *phIfft = NULL;

    return;
}

