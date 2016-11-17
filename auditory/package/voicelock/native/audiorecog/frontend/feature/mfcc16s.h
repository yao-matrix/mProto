#ifndef MFCC16S_H
#define MFCC16S_H

#include "AudioDef.h"
#include "AudioConfig.h"

#include "math/dct.h"

#define RATSA_ORDER   5
#define DELTA_ORDER   5
#define DDELTA_ORDER  9

#define DDELTA_BUFLEN 9

#if DDELTA_MFCC
    #define MFCC_DELAY    8
    #define RAW_BUFLEN    5
    #define RASTA_BUFLEN  7
    #define DELTA_BUFLEN  7
#elif (DELTA_MFCC || RASTA_MFCC)
    #define MFCC_DELAY    4
    #define RAW_BUFLEN    3
    #define RASTA_BUFLEN  5
    #define DELTA_BUFLEN  5
#else
    #define MFCC_DELAY    0
    #define RAW_BUFLEN    0
    #define RASTA_BUFLEN  5
    #define DELTA_BUFLEN  5
#endif

typedef struct
{
    AUD_Int32s   *pCentersFFT;
    AUD_Int32s   **pWeightsQ14;
    AUD_Int32s   *pWeightsLen;

    AUD_Int32s   FFTLen;
    AUD_Int32s   FFTOrder;
    AUD_Int32s   nFilter;
    AUD_Float    melMul;
    AUD_Float    melDiv;
    AUD_Int32s   Q;
} MelfbState;

void melfb_init( MelfbState **phFb, AUD_Int32s *pFFTLen, AUD_Int32s winSize,
                 AUD_Float sampFreq, AUD_Float lowFreq, AUD_Float highFreq, AUD_Int32s nFilter, AUD_Float melMul, AUD_Float melDiv );
void melfb_calc( MelfbState *pFb, AUD_Int32s *X, AUD_Int32s *fb );
void melfb_free( MelfbState **phFb );

typedef struct
{
    MelfbState      *pFb;
    Dct_16s         *pDct;
    AUD_Int32s      *fbBuffer;
    AUD_Int32s      fftLen;
    AUD_Int32s      fbLen;
    AUD_Int32s      mfccLen;
    AUD_AmpCompress compressType;

    AUD_Int32s      *pState[RAW_BUFLEN];
} Mfcc;

void mfcc_init( Mfcc **phMfcc, AUD_Int32s winSize, AUD_Float fs,
                AUD_Float fl, AUD_Float fh, AUD_Int32s fbLen, AUD_Float melMul, AUD_Float melDiv, AUD_Int32s mfccLen, AUD_AmpCompress compressType );
void mfcc_calc( Mfcc *pMfcc , AUD_Int32s *X, AUD_Int32s *mfcc );
void mfcc_get( Mfcc *pMfcc, AUD_Int32s *pOut );
void mfcc_free( Mfcc **phMfcc );


typedef struct
{
    AUD_Int32s   c0, c1, c2;
    AUD_Int32s   mfccLen;
} Mfcc_fdelta;

void mfccfd_init( Mfcc_fdelta **phMfd, AUD_Int32s mfccLen );
void mfccfd_calc( Mfcc_fdelta *pMfd, AUD_Int32s *pIn, AUD_Int32s *pOut );
void mfccfd_free( Mfcc_fdelta **phMfd );


typedef struct
{
    AUD_Int32s *pXState[RASTA_BUFLEN];
    AUD_Int32s *ys;
    AUD_Double a;
    AUD_Double b[DELTA_ORDER];
    AUD_Int32s mfccLen;
} Mfcc_rasta;

void mfccrasta_init( Mfcc_rasta **phMrasta, AUD_Int32s mfccLen );
void mfccrasta_calc( Mfcc_rasta *pMrasta, AUD_Int32s *pIn, AUD_Int32s *pOut );
void mfccrasta_refresh( Mfcc_rasta *pMrasta );
void mfccrasta_free( Mfcc_rasta **phMrasta );


typedef struct
{
    AUD_Int32s *pState[DELTA_BUFLEN];
    AUD_Double c[DDELTA_ORDER];
    AUD_Int32s mfccLen;
} Mfcc_delta;

void mfccd_init( Mfcc_delta **phMd, AUD_Int32s mfccLen );
void mfccd_calc( Mfcc_delta *pMd, AUD_Int32s *pIn, AUD_Int32s *pOut );
void mfccd_refresh( Mfcc_delta *pMd );
void mfccd_free( Mfcc_delta **phMd );


typedef struct
{
    AUD_Int32s *pState[DDELTA_BUFLEN];
    AUD_Double c[DDELTA_BUFLEN];
    AUD_Int32s mfccLen;
} Mfcc_deltadelta;

void mfccdd_init( Mfcc_deltadelta **phMdd, AUD_Int32s mfccLen );
void mfccdd_calc( Mfcc_deltadelta *pMdd, AUD_Int32s *pIn, AUD_Int32s *pOut );
void mfccdd_refresh( Mfcc_deltadelta *pMdd );
void mfccdd_free( Mfcc_deltadelta **phMdd );

#endif
