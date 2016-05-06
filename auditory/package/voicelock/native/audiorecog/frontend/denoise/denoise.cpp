/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "math/mmath.h"
#include "math/fft.h"

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"
#include "AudioConfig.h"

static AUD_Double berouti( AUD_Double snr )
{
    AUD_Double beta;

    if ( snr >= -5.0 && snr <= 20 )
    {
        beta = 4 - snr * 3. / 20.;
    }
    else if ( snr < -5.0 )
    {
        beta = 5.;
    }
    else
    {
        beta = 1.;
    }

    return beta;
}


AUD_Int32s denoise_mmse( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen )
{
    Fft_16s        *hFft        = NULL;
    Ifft_16s       *hIfft       = NULL;
    AUD_Window16s  *hWin        = NULL;

    AUD_Int32s     frameSize    = 512;
    AUD_Int32s     frameStride  = 256;
    AUD_Int32s     frameOverlap = 256;
    AUD_Int32s     nFFT         = frameSize;
    AUD_Int32s     nSpecLen     = nFFT / 2 + 1;
    AUD_Int32s     nNoiseFrame  = (AUD_Int32s)( ( 0.25 * SAMPLE_RATE - frameSize ) / frameStride + 1 );;

    AUD_Int32s     i, j, k;

    AUD_Int32s     cleanLen = 0;

    // pre-emphasis
    // sig_preemphasis( pInBuf, pInBuf, inLen );

    // init hamming module
    win16s_init( &hWin, AUD_WIN_HAMM, frameSize, 14 );
    AUD_ASSERT( hWin );

    // init fft handle
    fft_init( &hFft, nFFT, 15 );
    AUD_ASSERT( hFft );

    // init ifft handle
    ifft_init( &hIfft, nFFT, 15 );
    AUD_ASSERT( hIfft );

    AUD_Int16s *pFrame  = (AUD_Int16s*)calloc( frameSize * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pFrame );

    // FFT
    AUD_Int32s *pFFTMag  = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTMag );
    AUD_Int32s *pFFTRe   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTRe );
    AUD_Int32s *pFFTIm   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTIm );

    AUD_Int32s *pFFTCleanRe = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanRe );
    AUD_Int32s *pFFTCleanIm = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanIm );

    AUD_Double *pNoiseMean = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pNoiseMean );
    AUD_Int32s *pFFTCleanMag = (AUD_Int32s*)calloc( nSpecLen * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanMag );

    AUD_Int16s *pxOld   = (AUD_Int16s*)calloc( frameSize * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pxOld );
    AUD_Int16s *pxClean = (AUD_Int16s*)calloc( frameSize * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pxClean );


    for ( i = 0; (i < nNoiseFrame) && ( (i * frameStride + frameSize) < inLen ); i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameSize, pFrame );

        fft_mag( hFft, pFrame, frameSize, pFFTMag );

        for ( j = 0; j < nSpecLen; j++ )
        {
            pNoiseMean[j] += (AUD_Double)pFFTMag[j];
        }
    }
    // compute noise mean
    for ( j = 0; j < nSpecLen; j++ )
    {
        pNoiseMean[j] /= nNoiseFrame;
    }

    AUD_Double thres = 3.;
    AUD_Double alpha = 2.;
    AUD_Double flr   = 0.002;
    AUD_Double G     = 0.9;
    AUD_Double snr, beta;
    AUD_Double tmp;
    AUD_Double sigEnergy, noiseEnergy;

    k = 0;
    // start processing
    for ( i = 0; (i * frameStride + frameSize) < inLen; i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameStride, pFrame );

        fft_calc( hFft, pFrame, frameSize, pFFTRe, pFFTIm );

        // compute SNR
        sigEnergy   = 0.;
        noiseEnergy = 0.;
        for ( j = 0; j < nSpecLen; j++ )
        {
            tmp          = pFFTRe[j] / 32768. * pFFTRe[j] / 32768. + pFFTIm[j] / 32768. * pFFTIm[j] / 32768.;
            sigEnergy   += tmp;
            noiseEnergy += pNoiseMean[j] / 32768. * pNoiseMean[j] / 32768.;

            tmp = sqrt( tmp ) * 32768.;
            if ( tmp > MAX_INT32S )
            {
                AUD_ECHO
                pFFTMag[j] = MAX_INT32S;
            }
            else
            {
                pFFTMag[j] = (AUD_Int32s)round( tmp );
            }
        }

        snr  = 10. * log10( sigEnergy / noiseEnergy );
        beta = berouti( snr );

        // AUDLOG( "signal energy: %.2f, noise energy: %.2f, snr: %.2f, beta: %.2f\n", sigEnergy, noiseEnergy, snr, beta );

#if 0
        AUDLOG( "noisy FFT:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%d, ", pFFTMag[j] );
        }
        AUDLOG( "\n" );
#endif

        tmp = 0.;
        for ( j = 0; j < nSpecLen; j++ )
        {
            tmp  = AUD_MAX( pow( pFFTMag[j], alpha ) - beta * pow( pNoiseMean[j], alpha ), flr * pow( pNoiseMean[j], alpha ) );

            pFFTCleanMag[j] = (AUD_Int32s)round( pow( tmp, 1. / alpha ) );
        }

        // re-estimate noise
        if ( snr < thres )
        {
            AUD_Double tmpNoise;
            for ( j = 0; j < nSpecLen; j++ )
            {
                tmpNoise = G * pow( pNoiseMean[j], alpha ) + ( 1 - G ) * pow( pFFTMag[j], alpha );
                pNoiseMean[j] = pow( tmpNoise, 1. / alpha );
            }
        }

#if 0
        AUDLOG( "clean FFT:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%d, ", pFFTCleanMag[j] );
        }
        AUDLOG( "\n" );
#endif

        pFFTCleanRe[0] = pFFTCleanMag[0];
        pFFTCleanIm[0] = 0;
        AUD_Double costheta, sintheta;
        for ( j = 1; j < nSpecLen; j++ )
        {
            if ( pFFTMag[j] != 0 )
            {
                costheta = (AUD_Double)pFFTRe[j] / (AUD_Double)pFFTMag[j];
                sintheta = (AUD_Double)pFFTIm[j] / (AUD_Double)pFFTMag[j];
                pFFTCleanRe[nFFT - j] = pFFTCleanRe[j] = (AUD_Int32s)round( costheta * pFFTCleanMag[j] );
                pFFTCleanIm[j]        = (AUD_Int32s)round( sintheta * pFFTCleanMag[j] );
                pFFTCleanIm[nFFT - j] = -pFFTCleanIm[j];
            }
            else
            {
                pFFTCleanRe[nFFT - j] = pFFTCleanRe[j] = pFFTCleanMag[j];
                pFFTCleanIm[nFFT - j] = pFFTCleanIm[j] = 0;
            }
        }

#if 0
        AUDLOG( "clean FFT with phase:\n" );
        for ( j = 0; j < nFFT; j++ )
        {
            AUDLOG( "%d + j%d, ", pFFTCleanRe[j], pFFTCleanIm[j] );
        }
        AUDLOG( "\n" );
#endif

        ifft_real( hIfft, pFFTCleanRe, pFFTCleanIm, nFFT, pxClean );

#if 0
        AUDLOG( "clean speech:\n" );
        for ( j = 0; j < nFFT; j++ )
        {
            AUDLOG( "%d, ", pxClean[j] );
        }
        AUDLOG( "\n" );
#endif

        for ( j = 0; j < frameStride; j++ )
        {
            pOutBuf[k + j] = pxOld[j] + pxClean[j];
            pxOld[j]       = pxClean[frameOverlap + j];
        }

        k        += frameStride;
        cleanLen += frameStride;
    }

    // de-emphasis
    // sig_deemphasis( pOutBuf, pOutBuf, cleanLen );

    win16s_free( &hWin );
    fft_free( &hFft );
    ifft_free( &hIfft );

    free( pFrame );
    free( pNoiseMean );

    free( pFFTMag );
    free( pFFTRe );
    free( pFFTIm );

    free( pFFTCleanMag );
    free( pFFTCleanRe );
    free( pFFTCleanIm );

    free( pxOld );
    free( pxClean );

    return cleanLen;
}

typedef struct
{
    AUD_Double meanEn;
    AUD_Int32s nbSpeechFrame;
    AUD_Int32s hangOver;
} _VAD_Nest;

AUD_Int32s vad_nest( _VAD_Nest *pState, AUD_Int16s *pBuf, AUD_Int32s len )
{
    AUD_Double frameEn;
    AUD_Double lambdaLTE = 0.97;
    AUD_Int32s vadFlag   = 0;
    AUD_Int32s i         = 0;

    AUD_Double x = 0.;
    for ( i = 0; i < len; i++ )
    {
        x += pow( pBuf[i], 2 );
    }

    frameEn = 16. * log( 1 + x / 64. ) / log( 2 );

    // update noise mean energy
    if ( frameEn - pState->meanEn < 20 )
    {
        if ( frameEn < pState->meanEn )
        {
            pState->meanEn += ( 1 - lambdaLTE ) * ( frameEn - pState->meanEn );
        }
        else
        {
            pState->meanEn += 0.01 * ( frameEn - pState->meanEn );
        }

        if ( pState->meanEn < 280 )
        {
            pState->meanEn = 280;
        }
    }

    if ( frameEn - pState->meanEn > 15 )
    {
        vadFlag = 1;
        pState->nbSpeechFrame++;
    }
    else
    {
        if ( pState->nbSpeechFrame > 4 )
        {
            pState->hangOver = 15;
        }
        pState->nbSpeechFrame = 0;

        if ( pState->hangOver != 0 )
        {
            pState->hangOver--;
            vadFlag = 1;
        }
        else
        {
            vadFlag = 0;
        }
    }

    return vadFlag;
}

AUD_Int32s denoise_wiener( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen )
{
    Fft_16s        *hFft        = NULL;
    Ifft_16s       *hIfft       = NULL;
    AUD_Window16s  *hWin        = NULL;
    _VAD_Nest      vadState;

    AUD_Int32s     frameSize    = 512;
    AUD_Int32s     frameStride  = 256;
    AUD_Int32s     nFFT         = frameSize;
    AUD_Int32s     nSpecLen     = nFFT / 2 + 1;
    AUD_Int32s     nNoiseFrame  = (AUD_Int32s)( ( 0.25 * SAMPLE_RATE - frameSize ) / frameStride + 1 );

    AUD_Int32s     i, j, k;

    AUD_Int32s     cleanLen = 0;

    vadState.meanEn        = 280;
    vadState.nbSpeechFrame = 0;
    vadState.hangOver      = 0;

    AUD_Int16s *pFrame  = (AUD_Int16s*)calloc( frameSize * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pFrame );

    AUD_Double *pNoiseMean = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pNoiseMean );

    AUD_Double *pLamdaD = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pLamdaD );

    AUD_Double *pXi = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pXi );

    AUD_Double *pGamma = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pGamma );

    AUD_Double *pG = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pG );

    AUD_Double *pGammaNew = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pGammaNew );

    for ( j = 0; j < nSpecLen; j++ )
    {
        pG[j]         = 1.;
        pGamma[j]     = 1.;
    }

    // FFT
    AUD_Int32s *pFFTMag  = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTMag );
    AUD_Int32s *pFFTRe   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTRe );
    AUD_Int32s *pFFTIm   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTIm );

    AUD_Double *pFFTCleanMag = (AUD_Double*)calloc( nFFT * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pFFTCleanMag );

    AUD_Int32s *pFFTCleanRe = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanRe );
    AUD_Int32s *pFFTCleanIm = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanIm );

    AUD_Int16s *pxClean = (AUD_Int16s*)calloc( nFFT * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pxClean );

    memset( pOutBuf, 0, inLen * sizeof(AUD_Int16s) );

    // init hamming module
    win16s_init( &hWin, AUD_WIN_HAMM, frameSize, 14 );
    AUD_ASSERT( hWin );

    // init fft handle
    fft_init( &hFft, nFFT, 15 );
    AUD_ASSERT( hFft );

    // init ifft handle
    ifft_init( &hIfft, nFFT, 15 );
    AUD_ASSERT( hIfft );

    // noise manipulation
    for ( i = 0; (i < nNoiseFrame) && ( (i * frameStride + frameSize) < inLen ); i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameStride, pFrame );

        fft_mag( hFft, pFrame, frameSize, pFFTMag );

        for ( j = 0; j < nSpecLen; j++ )
        {
            pNoiseMean[j] += (AUD_Double)pFFTMag[j];
            pLamdaD[j]    += (AUD_Double)pFFTMag[j] * (AUD_Double)pFFTMag[j];
        }
    }
    // compute noise mean
    for ( j = 0; j < nSpecLen; j++ )
    {
        pNoiseMean[j] /= nNoiseFrame;
        pLamdaD[j]    /= nNoiseFrame;
    }

    AUD_Int32s vadFlag  = 0;
    AUD_Double noiseLen = 9.;
    AUD_Double alpha    = 0.99;
    k = 0;
    for ( i = 0; (i * frameStride + frameSize) < inLen; i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameStride, pFrame );

        fft_calc( hFft, pFrame, frameSize, pFFTRe, pFFTIm );

        for ( j = 0; j < nSpecLen; j++ )
        {
            pFFTMag[j] = (AUD_Int32s)round( sqrt( (AUD_Double)pFFTRe[j] * pFFTRe[j] + (AUD_Double)pFFTIm[j] * pFFTIm[j] ) );
        }

#if 0
        AUDLOG( "noisy FFT:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%d, ", pFFTMag[j] );
        }
        AUDLOG( "\n" );
#endif

        vadFlag = vad_nest( &vadState, pFrame, frameSize );
        if ( vadFlag == 0 )
        {
            for ( j = 0; j < nSpecLen; j++ )
            {
                pNoiseMean[j] = ( noiseLen * pNoiseMean[j] + (AUD_Double)pFFTMag[j] ) / ( noiseLen + 1. );
                pLamdaD[j]    = ( noiseLen * pLamdaD[j] + (AUD_Double)pFFTMag[j] * pFFTMag[j] ) / ( noiseLen + 1. );
            }
        }

        for ( j = 0; j < nSpecLen; j++ )
        {
            pGammaNew[j] = (AUD_Double)pFFTMag[j] * pFFTMag[j] / pLamdaD[j];
            pXi[j]       = alpha * pG[j] * pG[j] * pGamma[j] + ( 1. - alpha ) * AUD_MAX( pGammaNew[j] - 1., 0. );
            pGamma[j]    = pGammaNew[j];
            pG[j]        = pXi[j]  / ( pXi[j] + 1. );

            pFFTCleanMag[j] = pG[j] * pFFTMag[j];
        }

#if 0
        AUDLOG( "clean FFT:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pFFTCleanMag[j] );
        }
        AUDLOG( "\n" );
#endif

        // combine to real/im part of IFFT
        pFFTCleanRe[0] = pFFTCleanMag[0];
        pFFTCleanIm[0] = 0;
        AUD_Double costheta, sintheta;
        for ( j = 1; j < nSpecLen; j++ )
        {
            if ( pFFTMag[j] != 0 )
            {
                costheta = (AUD_Double)pFFTRe[j] / (AUD_Double)pFFTMag[j];
                sintheta = (AUD_Double)pFFTIm[j] / (AUD_Double)pFFTMag[j];
                pFFTCleanRe[nFFT - j] = pFFTCleanRe[j] = (AUD_Int32s)round( costheta * pFFTCleanMag[j] );
                pFFTCleanIm[j] = (AUD_Int32s)round( sintheta * pFFTCleanMag[j] );
                pFFTCleanIm[nFFT - j] = -pFFTCleanIm[j];
            }
            else
            {
                pFFTCleanRe[nFFT - j] = pFFTCleanRe[j] = pFFTCleanMag[j];
                pFFTCleanIm[nFFT - j] = pFFTCleanIm[j] = 0;
            }
        }

        ifft_real( hIfft, pFFTCleanRe, pFFTCleanIm, nFFT, pxClean );

#if 0
        AUDLOG( "clean FFT with phase:\n" );
        for ( j = 0; j < nFFT; j++ )
        {
            AUDLOG( "%d + j%d, ", pFFTCleanRe[j], pFFTCleanIm[j] );
        }
        AUDLOG( "\n" );
#endif

        for ( j = 0; j < frameSize; j++ )
        {
            pOutBuf[k + j] = pOutBuf[k + j] + pxClean[j];
        }

        k        += frameStride;
        cleanLen += frameStride;
    }

    win16s_free( &hWin );
    fft_free( &hFft );
    ifft_free( &hIfft );

    free( pFrame );
    free( pNoiseMean );
    free( pLamdaD );
    free( pXi );
    free( pGamma );
    free( pG );
    free( pGammaNew );

    free( pFFTMag );
    free( pFFTRe );
    free( pFFTIm );

    free( pFFTCleanMag );
    free( pFFTCleanRe );
    free( pFFTCleanIm );

    free( pxClean );

    return cleanLen;
}

#define CRITICAL_BAND_NUM 22
AUD_Int32s denoise_aud( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen )
{
    Fft_16s        *hFft        = NULL;
    Ifft_16s       *hIfft       = NULL;
    AUD_Window16s  *hWin        = NULL;

    AUD_Int32s     frameSize    = 512;
    AUD_Int32s     frameStride  = 256;
    AUD_Int32s     frameOverlap = 256;
    AUD_Int32s     nFFT         = frameSize;
    AUD_Int32s     nSpecLen     = nFFT / 2 + 1;
    AUD_Int32s     nNoiseFrame  = 6; // (AUD_Int32s)( ( 0.25 * SAMPLE_RATE - frameSize ) / frameStride + 1 );

    AUD_Int32s     i, j, k, m, n, ret;

    AUD_Int32s     cleanLen     = 0;

    // pre-emphasis
    // sig_preemphasis( pInBuf, pInBuf, inLen );

    // init hamming module
    win16s_init( &hWin, AUD_WIN_HAMM, frameSize, 14 );
    AUD_ASSERT( hWin );

    // init fft handle
    fft_init( &hFft, nFFT, 15 );
    AUD_ASSERT( hFft );

    // init ifft handle
    ifft_init( &hIfft, nFFT, 15 );
    AUD_ASSERT( hIfft );

    AUD_Int16s *pFrame  = (AUD_Int16s*)calloc( frameSize * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pFrame );

    // FFT
    AUD_Int32s *pFFTMag  = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTMag );
    AUD_Int32s *pFFTRe   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTRe );
    AUD_Int32s *pFFTIm   = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTIm );

    AUD_Int32s *pFFTCleanRe = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanRe );
    AUD_Int32s *pFFTCleanIm = (AUD_Int32s*)calloc( nFFT * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pFFTCleanIm );

    // noise spectrum
    AUD_Double *pNoiseEn = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pNoiseEn );

    AUD_Double *pNoiseB = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pNoiseB );

    AUD_Double *pXPrev = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pXPrev );

    AUD_Double *pAb = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pAb );

    AUD_Double *pH = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pH );

    AUD_Double *pGammak = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pGammak );

    AUD_Double *pKsi = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pKsi );

    AUD_Double *pLogSigmak = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pLogSigmak );

    AUD_Double *pAlpha = (AUD_Double*)calloc( nSpecLen * sizeof(AUD_Double), 1 );
    AUD_ASSERT( pAlpha );

    AUD_Int32s *pLinToBark = (AUD_Int32s*)calloc( nSpecLen * sizeof(AUD_Int32s), 1 );
    AUD_ASSERT( pLinToBark );

    AUD_Int16s *pxOld   = (AUD_Int16s*)calloc( frameOverlap * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pxOld );
    AUD_Int16s *pxClean = (AUD_Int16s*)calloc( nFFT * sizeof(AUD_Int16s), 1 );
    AUD_ASSERT( pxClean );

    /*
    AUD_Int32s critBandEnds[22] = { 0, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270,
                                    1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700 };
    */
    AUD_Int32s critFFTEnds[CRITICAL_BAND_NUM + 1]  = { 0, 4, 7, 10, 13, 17, 21, 25, 30, 35, 41, 48, 56, 64,
                                                       75, 87, 101, 119, 141, 170, 205, 247, 257 };

    // generate linear->bark transform mapping
    k = 0;
    for ( i = 0; i < CRITICAL_BAND_NUM; i++ )
    {
        while ( k >= critFFTEnds[i] &&  k < critFFTEnds[i + 1] )
        {
            pLinToBark[k] = i;
            k++;
        }
    }

    AUD_Double absThr[CRITICAL_BAND_NUM] = { 38, 31, 22, 18.5, 15.5, 13, 11, 9.5, 8.75, 7.25, 4.75, 2.75,
                                             1.5, 0.5, 0, 0, 0, 0, 2, 7, 12, 15.5 };

    AUD_Double dbOffset[CRITICAL_BAND_NUM];
    AUD_Double sumn[CRITICAL_BAND_NUM];
    AUD_Double spread[CRITICAL_BAND_NUM];

    for ( i = 0; i < CRITICAL_BAND_NUM; i++ )
    {
        absThr[i]   = pow( 10., absThr[i] / 10. ) / nFFT / ( 65535. * 65535. );
        dbOffset[i] = 10. + i;

        sumn[i]   = 0.474 + i;
        spread[i] = pow( 10., ( 15.81 + 7.5 * sumn[i] - 17.5 * sqrt( 1. + sumn[i] * sumn[i] ) ) / 10. );
    }

    AUD_Double dcGain[CRITICAL_BAND_NUM];
    for ( i = 0; i < CRITICAL_BAND_NUM; i++ )
    {
        dcGain[i] = 0.;
        for ( j = 0; j < CRITICAL_BAND_NUM; j++ )
        {
            dcGain[i] += spread[MABS( i - j )];
        }
    }

    AUD_Matrix exPatMatrix;
    exPatMatrix.rows     = CRITICAL_BAND_NUM;
    exPatMatrix.cols     = nSpecLen;
    exPatMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &exPatMatrix );
    AUD_ASSERT( ret == 0 );

    // excitation pattern
    AUD_Int32s index = 0;
    for ( i = 0; i < exPatMatrix.rows; i++ )
    {
        AUD_Double *pExpatRow = exPatMatrix.pDouble + i * exPatMatrix.cols;

        for ( j = 0; j < exPatMatrix.cols; j++ )
        {
            index = MABS( i - pLinToBark[j] );
            pExpatRow[j] = spread[index];
        }
    }

    AUD_Int32s frameNum = (inLen - frameSize) / frameStride + 1;
    AUD_ASSERT( frameNum > nNoiseFrame );

    // compute noise mean
    for ( i = 0; i < nNoiseFrame; i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameSize, pFrame );

        fft_mag( hFft, pFrame, frameSize, pFFTMag );

        for ( j = 0; j < nSpecLen; j++ )
        {
            pNoiseEn[j] += pFFTMag[j] / 32768. * pFFTMag[j] / 32768.;
        }
    }
    for ( j = 0; j < nSpecLen; j++ )
    {
        pNoiseEn[j] /= nNoiseFrame;
    }

    // get cirtical band mean filtered noise power
    AUD_Int32s k1 = 0, k2 = 0;
    for ( i = 0; i < CRITICAL_BAND_NUM; i++ )
    {
        k1 = k2;
        AUD_Double segSum = 0.;
        while ( k2 >= critFFTEnds[i] &&  k2 < critFFTEnds[i + 1] )
        {
            segSum += pNoiseEn[k2];
            k2++;
        }
        segSum /= ( k2 - k1 );

        for ( m = k1; m < k2; m++ )
        {
            pNoiseB[m] = segSum;
        }
    }

#if 0
    AUDLOG( "noise band spectrum:\n" );
    for ( j = 0; j < nSpecLen; j++ )
    {
        AUDLOG( "%.2f, ", pNoiseB[j] );
    }
    AUDLOG( "\n" );
#endif

    AUD_Matrix frameMatrix;
    frameMatrix.rows     = nSpecLen;
    frameMatrix.cols     = 1;
    frameMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &frameMatrix );
    AUD_ASSERT( ret == 0 );
    AUD_Double *pFrameEn = frameMatrix.pDouble;

    AUD_Matrix xMatrix;
    xMatrix.rows     = nSpecLen;
    xMatrix.cols     = 1;
    xMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &xMatrix );
    AUD_ASSERT( ret == 0 );
    AUD_Double *pX = xMatrix.pDouble;

    AUD_Matrix cMatrix;
    cMatrix.rows     = CRITICAL_BAND_NUM;
    cMatrix.cols     = 1;
    cMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &cMatrix );
    AUD_ASSERT( ret == 0 );
    AUD_Double *pC = cMatrix.pDouble;

    AUD_Matrix tMatrix;
    tMatrix.rows     = 1;
    tMatrix.cols     = CRITICAL_BAND_NUM;
    tMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &tMatrix );
    AUD_ASSERT( ret == 0 );
    AUD_Double *pT = tMatrix.pDouble;

    AUD_Matrix tkMatrix;
    tkMatrix.rows     = 1;
    tkMatrix.cols     = nSpecLen;
    tkMatrix.dataType = AUD_DATATYPE_DOUBLE;
    ret = createMatrix( &tkMatrix );
    AUD_ASSERT( ret == 0 );
    AUD_Double *pTk = tkMatrix.pDouble;

    AUD_Double dB0[CRITICAL_BAND_NUM];
    AUD_Double epsilon = pow( 2, -52 );

    #define ESTIMATE_MASKTHRESH( sigMatrix, tkMatrix )\
    do {\
        AUD_Double *pSig = sigMatrix.pDouble; \
        for ( m = 0; m < exPatMatrix.rows; m++ ) \
        { \
            AUD_Double suma = 0.; \
            AUD_Double *pExpatRow = exPatMatrix.pDouble + m * exPatMatrix.cols; \
            for ( n = 0; n < exPatMatrix.cols; n++ ) \
            { \
                suma += pExpatRow[n] * pSig[n]; \
            } \
            pC[m] = suma; \
        } \
        AUD_Double product = 1.; \
        AUD_Double sum     = 0.; \
        for ( m = 0; m < sigMatrix.rows; m++ ) \
        { \
            product *= pSig[m]; \
            sum     += pSig[m]; \
        } \
        AUD_Double power = 1. / sigMatrix.rows;\
        AUD_Double sfmDB = 10. * log10( pow( product, power ) / sum / sigMatrix.rows + epsilon ); \
        AUD_Double alpha = AUD_MIN( 1., sfmDB / (-60.) ); \
        for ( m = 0; m < tMatrix.cols; m++ ) \
        { \
            dB0[m] = dbOffset[m] * alpha + 5.5; \
            pT[m]  = pC[m] / pow( 10., dB0[m] / 10. ) / dcGain[m]; \
            pT[m]  = AUD_MAX( pT[m], absThr[m] ); \
        } \
        for ( m = 0; m < tkMatrix.cols; m++ ) \
        { \
            pTk[m] = pT[pLinToBark[m]]; \
        } \
       } while ( 0 )

    AUD_Double aa    = 0.98;
    AUD_Double mu    = 0.98;
    AUD_Double eta   = 0.15;
    AUD_Double vadDecision;

    k = 0;
    // start processing
    for ( i = 0; i < frameNum; i++ )
    {
        win16s_calc( hWin, pInBuf + i * frameStride, pFrame );

        fft_calc( hFft, pFrame, frameSize, pFFTRe, pFFTIm );

        // compute SNR
        vadDecision = 0.;
        for ( j = 0; j < nSpecLen; j++ )
        {
            pFrameEn[j] = pFFTRe[j] / 32768. * pFFTRe[j] / 32768. + pFFTIm[j] / 32768. * pFFTIm[j] / 32768.;

            pGammak[j]  = AUD_MIN( pFrameEn[j] / pNoiseEn[j], 40. );

            if ( i > 0 )
            {
                pKsi[j] = aa * pXPrev[j] / pNoiseEn[j] + ( 1 - aa ) * AUD_MAX( pGammak[j] - 1., 0. );
            }
            else
            {
                pKsi[j] = aa + ( 1. - aa ) * AUD_MAX( pGammak[j] - 1., 0. );
            }

            pLogSigmak[j] = pGammak[j] * pKsi[j] / ( 1. + pKsi[j] ) - log( 1. + pKsi[j] );

            vadDecision += ( j > 0 ? 2 : 1 ) * pLogSigmak[j];
        }
        vadDecision /= nFFT;

#if 0
        AUDLOG( "X prev:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pXPrev[j] );
        }
        AUDLOG( "\n" );
#endif

#if 0
        AUDLOG( "gamma k:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pGammak[j] );
        }
        AUDLOG( "\n" );
#endif

#if 0
        AUDLOG( "ksi:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pKsi[j] );
        }
        AUDLOG( "\n" );
#endif

#if 0
        AUDLOG( "log sigma k:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pLogSigmak[j] );
        }
        AUDLOG( "\n" );
#endif

        // AUDLOG( "vadDecision: %.2f\n", vadDecision );
        // re-estimate noise
        if ( vadDecision < eta )
        {
            for ( j = 0; j < nSpecLen; j++ )
            {
                pNoiseEn[j] = mu * pNoiseEn[j] + ( 1. - mu ) * pFrameEn[j];
            }

            // re-estimate crital band based noise
            AUD_Int32s k1 = 0, k2 = 0;
            for ( int band = 0; band < CRITICAL_BAND_NUM; band++ )
            {
                k1 = k2;
                AUD_Double segSum = 0.;
                while ( k2 >= critFFTEnds[band] &&  k2 < critFFTEnds[band + 1] )
                {
                    segSum += pNoiseEn[k2];
                    k2++;
                }
                segSum /= ( k2 - k1 );

                for ( m = k1; m < k2; m++ )
                {
                    pNoiseB[m] = segSum;
                }
            }

#if 0
            AUDLOG( "noise band spectrum:\n" );
            for ( j = 0; j < nSpecLen; j++ )
            {
                AUDLOG( "%.2f, ", pNoiseB[j] );
            }
            AUDLOG( "\n" );
#endif
        }

        for ( j = 0; j < nSpecLen; j++ )
        {
            pX[j]     = AUD_MAX( pFrameEn[j] - pNoiseEn[j], 0.001 );
            pXPrev[j] = pFrameEn[j];
        }

        ESTIMATE_MASKTHRESH( xMatrix, tkMatrix );
        for ( int iter = 0; iter < 2; iter++ )
        {
            for ( j = 0; j < nSpecLen; j++ )
            {
                pAb[j]      = pNoiseB[j] + pNoiseB[j] * pNoiseB[j] / pTk[j];
                pFrameEn[j] = pFrameEn[j] * pFrameEn[j] / ( pFrameEn[j] + pAb[j] );
                ESTIMATE_MASKTHRESH( frameMatrix, tkMatrix );

#if 0
                showMatrix( &tMatrix );
#endif
            }
        }

#if 0
        AUDLOG( "tk:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pTk[j] );
        }
        AUDLOG( "\n" );
#endif

        pAlpha[0]      = ( pNoiseB[0] + pTk[0] ) * ( pNoiseB[0] / pTk[0] );
        pH[0]          = pFrameEn[0] / ( pFrameEn[0] + pAlpha[0] );
        pXPrev[0]     *= pH[0] * pH[0];
        pFFTCleanRe[0] = 0;
        pFFTCleanIm[0] = 0;
        for ( j = 1; j < nSpecLen; j++ )
        {
            pAlpha[j] = ( pNoiseB[j] + pTk[j] ) * ( pNoiseB[j] / pTk[j] );

            pH[j]     = pFrameEn[j] / ( pFrameEn[j] + pAlpha[j] );

            pFFTCleanRe[j] = pFFTCleanRe[nFFT - j] = (AUD_Int32s)round( pH[j] * pFFTRe[j] );
            pFFTCleanIm[j]        = (AUD_Int32s)round( pH[j] * pFFTIm[j] );
            pFFTCleanIm[nFFT - j] = -pFFTCleanIm[j];

            pXPrev[j] *= pH[j] * pH[j];
        }

#if 0
        AUDLOG( "denoise transfer function:\n" );
        for ( j = 0; j < nSpecLen; j++ )
        {
            AUDLOG( "%.2f, ", pH[j] );
        }
        AUDLOG( "\n" );
#endif

#if 0
        AUDLOG( "clean FFT with phase:\n" );
        for ( j = 0; j < nFFT; j++ )
        {
            AUDLOG( "%d + j%d, ", pFFTCleanRe[j], pFFTCleanIm[j] );
        }
        AUDLOG( "\n" );
#endif

        ifft_real( hIfft, pFFTCleanRe, pFFTCleanIm, nFFT, pxClean );

#if 0
        AUDLOG( "clean speech:\n" );
        for ( j = 0; j < nFFT; j++ )
        {
            AUDLOG( "%d, ", pxClean[j] );
        }
        AUDLOG( "\n" );
#endif

        for ( j = 0; j < frameStride; j++ )
        {
            if ( j < frameOverlap )
            {
                pOutBuf[k + j] = pxOld[j] + pxClean[j];
                pxOld[j] = pxClean[frameStride + j];
            }
            else
            {
                pOutBuf[k + j] = pxClean[j];
            }
        }

        k        += frameStride;
        cleanLen += frameStride;
    }

    // de-emphasis
    // sig_deemphasis( pOutBuf, pOutBuf, cleanLen );

    win16s_free( &hWin );
    fft_free( &hFft );
    ifft_free( &hIfft );

    free( pFrame );
    free( pNoiseEn );
    free( pNoiseB );

    free( pFFTMag );
    free( pFFTRe );
    free( pFFTIm );
    free( pXPrev );
    free( pAb );
    free( pH );

    free( pFFTCleanRe );
    free( pFFTCleanIm );

    free( pxOld );
    free( pxClean );

    free( pGammak );
    free( pKsi );
    free( pLogSigmak );
    free( pAlpha );
    free( pLinToBark );

    ret = createMatrix( &xMatrix );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &exPatMatrix );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &frameMatrix );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &cMatrix );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &tMatrix );
    AUD_ASSERT( ret == 0 );

    ret = destroyMatrix( &tkMatrix );
    AUD_ASSERT( ret == 0 );

    return cleanLen;
}

