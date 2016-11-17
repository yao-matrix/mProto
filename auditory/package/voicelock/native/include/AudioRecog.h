#if !defined( _AUD_RECOG_H )
#define _AUD_RECOG_H

#include <stdio.h>

#include "AudioDef.h"

#include "type/matrix.h"
#include "math/mmath.h"

/*******************************************************************************************
 * Voice Activity Detection
 ******************************************************************************************/
AUD_Int32s sig_vadinit( void **phVadHandle, AUD_Int16s *pSpeechBuf, AUD_Int16s *pSpeechFlag, AUD_Int32s speechBufLen, AUD_Int32s backTrackDepth, AUD_Int32s tailDepth );
AUD_Int32s sig_vadupdate( void *hVadHandle, AUD_Int16s *pFrameBuf, AUD_Int32s *pStart );
AUD_Int32s sig_framecheck( void *hVadHandle, AUD_Int16s *pFrameBuf, AUD_Int16s *pFlag ); /* hide */
AUD_Int32s sig_vaddeinit( void **phVadHandle );


/*******************************************************************************************
 * Pre-processing
 ******************************************************************************************/
#define AUD_VAD_ENERGY 0
#define AUD_VAD_GMM    1

void sig_normalize( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len );
void sig_preemphasis( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len );
void sig_deemphasis( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s len );


/*******************************************************************************************
 * Denoise
 ******************************************************************************************/
AUD_Int32s denoise_mmse( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen );
AUD_Int32s denoise_wiener( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen );
AUD_Int32s denoise_aud( AUD_Int16s *pInBuf, AUD_Int16s *pOutBuf, AUD_Int32s inLen );

/*******************************************************************************************
 * Frame Admisson
 ******************************************************************************************/
AUD_Int32s sig_frameadmit( AUD_Int16s *pInBuf, AUD_Int32s inSampleNum, AUD_Int16s *pOutBuf );


/*******************************************************************************************
 * Windowing
 ******************************************************************************************/
// smooth window
typedef enum
{
    AUD_WIN_HANN      = 0,
    AUD_WIN_HAMM      = 1,

    AUD_WIN_LIMIT     = 0x7fffffff,
} AUD_WindowType;

typedef struct
{
    AUD_WindowType type;
    AUD_Int32s     len;
    AUD_Int32s     Q;
    AUD_Int16s     *h;
} AUD_Window16s;

AUD_Error win16s_init( AUD_Window16s **phWin, AUD_WindowType type, AUD_Int32s len, AUD_Int32s Q );
AUD_Error win16s_calc( AUD_Window16s *hWin, AUD_Int16s *x, AUD_Int16s *y );
AUD_Error win16s_free( AUD_Window16s **phWin );


/*******************************************************************************************
 * Feature Extraction
 ******************************************************************************************/
typedef struct
{
    AUD_Matrix  featureMatrix;
    AUD_Vector  featureNorm;
} AUD_Feature;

// pitch
AUD_Error pitch_track( AUD_Int16s *pInBuf, AUD_Int32s len, AUD_Int32s *pPitch );

// spectrogram
AUD_Error spectrogram16s32s_init( void **phSpecHandle,
                                  AUD_Int32s winLen, AUD_WindowType winType, AUD_Int32s frameStride, AUD_AmpCompress compressType );

AUD_Error spectrogram16s32s_calc( void *hSpecHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                                  AUD_Matrix *pOutSpectrogram, AUD_Int32s *pOutTimeSlices, AUD_Bool *pIsRewind );

AUD_Error spectrogram16s32s_deinit( void **phSpecHandle );

// Mel-Frequency Cepstral Coefficients
AUD_Error mfcc16s32s_init( void **phMfccHandle,
                           AUD_Int32s winLen, AUD_WindowType winType, AUD_Int16s featureOrder, AUD_Int32s frameStride, AUD_Int32s sampleRate, AUD_AmpCompress compressType );

AUD_Error mfcc16s32s_calc( void *hMfccHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                           AUD_Feature *pOutMFCC );

AUD_Error mfcc16s32s_refresh( void *hMfccHandle );

AUD_Error mfcc16s32s_deinit( void **phMfccHandle );

// Time Frequency Image
AUD_Error tfi16s32s_init( void **phTfiHandle,
                          AUD_Int32s winLen, AUD_WindowType winType, AUD_Int32s frameStride, AUD_AmpCompress compressType );

AUD_Error tfi16s32s_calc( void *hTfiHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                          AUD_Feature *pOutTFI );

AUD_Error tfi16s32s_deinit( void **phTfiHandle );


/*******************************************************************************************
 * Recognition
 ******************************************************************************************/
typedef enum
{
    AUD_DISTTYPE_COSINE       = 0,
    AUD_DISTTYPE_EUCLIDEAN    = 1,
    AUD_DISTTYPE_MANHATTAN    = 2,
    // AUD_DISTTYPE_KLDIVERGENCE = 3,

    AUD_DISTTYPE_LIMIT      = 0x7fffffff,
} AUD_DistType;


/*-----------------------------------------------------------------------------------------
 - Template Matching
 -----------------------------------------------------------------------------------------*/

// Dynamic Time Warp
typedef enum
{
    AUD_DTWTYPE_CLASSIC     = 0,
    AUD_DTWTYPE_SUBSEQUENCE = 1,
    // AUD_DTWTYPE_MULTISCALE  = 2,

    AUD_DTWTYPE_LIMIT       = 0x7fffffff,
} AUD_DTWType;

// DTW transition type
typedef enum
{
    AUD_DTWTRANSITION_LEVENSHTEIN = 0,
    AUD_DTWTRANSITION_DTW         = 1,

    AUD_DTWTRANSITION_LIMIT       = 0x7fffffff,
} AUD_DTWTransition;

// DTW scoring method
typedef enum
{
    AUD_DTWSCORE_BEST     = 0,
    AUD_DTWSCORE_TERMINAL = 1,

    AUD_DTWSCORE_LIMIT    = 0x7fffffff,
} AUD_DTWScoring;

typedef struct
{
    AUD_DTWType        dtwType;
    AUD_DistType       distType;
    AUD_DTWTransition  transitionType;
    AUD_Feature        *pTemplate;
    AUD_CircularMatrix frameCost;
    AUD_Matrix         dtw;
    AUD_Double         *pWorkBuf;
} AUD_DTWSession;

AUD_Error dtw_initsession( AUD_DTWSession *pDTWSession, AUD_Feature *pTemplate, AUD_Int32s inputFeatureNum );
AUD_Error dtw_updatefrmcost( AUD_DTWSession *pDTWSession, AUD_Feature *pNewFeature );
AUD_Error dtw_refreshfrmcost( AUD_DTWSession *pDTWSession );
AUD_Error dtw_match( AUD_DTWSession *pDTWSession, AUD_DTWScoring scoringMethod, AUD_Double *pScore, AUD_Vector *pPath );
AUD_Error dtw_deinitsession( AUD_DTWSession *pDTWSession );


/*------------------------------------------------------------------------------------------
 - Stochastical Model
 ------------------------------------------------------------------------------------------*/
// k-means
// #if defined( ENABLE_OPENCV )
AUD_Error  kmeans_cluster( AUD_Matrix *pSamples, AUD_Int32s numCluster, AUD_Int32s maxInteration, AUD_Double minEps, AUD_Int32s attempts, AUD_Matrix *pClusterLabel, AUD_Matrix *pClusterCentroid );
// #endif

AUD_Double calc_nearestdist( AUD_Matrix *pFeatMatrix, AUD_Int32s rowIndex, AUD_Matrix *pCluster );

// PCA
// #if defined( ENABLE_OPENCV )
AUD_Error pca_calc( AUD_Matrix *pSamples, AUD_Int32s maxComponents, AUD_Matrix *pAvg, AUD_Matrix *pEigenVec );
// #endif

AUD_Error pca_project( AUD_Matrix *pSamples, AUD_Matrix *pAvg, AUD_Matrix *pEigenVec, AUD_Matrix *pOutPca );

// Gaussian Mixture Model
#define GMM_INVERTSELECT_MASK  0x1

AUD_Error  gmm_init( void **phGmmHandle, AUD_Int8s *pModelName, AUD_Int32s numMix, AUD_Int32s featDim, AUD_Vector *pWeights, AUD_Matrix *pMeans, AUD_Matrix *pCvars );
AUD_Error  gmm_clone( void **phGmmHandle, void *hSrcGmmHandle, AUD_Int8s *pName );
AUD_Error  gmm_mix( void **phGmmHandle, void *hSrcGmmHandle1, void *hSrcGmmHandle2, AUD_Int8s *pName );
AUD_Error  gmm_import( void **phGmmHandle, FILE *fp );
AUD_Error  gmm_select( void **phGmmHandle, void *hSrcGmmHandle, AUD_Vector *pIndex, AUD_Int32s selectMode, AUD_Int8s *pName );
AUD_Error  gmm_train( void **phGmmHandle, AUD_Matrix *pSamples, AUD_Int8s *pGmmName, AUD_Int32s numMix, AUD_Int32s maxIter, AUD_Double minEps );

AUD_Error  gmm_adapt( void *hGmmHandle, AUD_Matrix *pAdaptData, bool &abortSignal );

AUD_Error  gmm_export( void *hGmmHandle, FILE *fp );

AUD_Error  gmm_show( void *hGmmHandle );

AUD_Error  gmm_free( void **phGmmHandle );

AUD_Double gmm_llr( void *hGmmHandle, AUD_Matrix *pFeature, AUD_Int32s sampleIndex, AUD_Vector *pComponentLLR );
AUD_Double gmm_prob( void *hGmmHandle, AUD_Matrix *pFeature, AUD_Int32s sampleIndex, AUD_Vector *pComponentProb );

AUD_Error  gmm_cluster( void *hGmmModel, AUD_Matrix *pIndexTable, AUD_Double threshold, AUD_Int32s *pClusterLabel );
AUD_Error  gmm_getname( void *hGmmHandle, AUD_Int8s arGmmName[256] );
AUD_Int32s gmm_getmixnum( void *hGmmHandle );


// gmm-hmm( Hidden Markov Model with the obeservation obey Gaussian Mixture Model )
/* GMM-HMM */
AUD_Error gmmhmm_init( void **phGmmHmm, AUD_Int32s stateNum, AUD_Vector *pPi, void **ppSrcGmmModels );
AUD_Error gmmhmm_learn( void *hGmmHmm, AUD_Feature *pFeatures, AUD_Int32s instanceNum, AUD_Double threshold );
AUD_Error gmmhmm_forward( void *hGmmHmm, AUD_Matrix *pSamples, AUD_Double *pProb );
AUD_Error gmmhmm_free( void **phGmmHmm );

AUD_Error gmmhmm_show( void *hGmmHmm );

AUD_Error gmmhmm_import( void **phGmmHmm, AUD_Int8s *pFileName );
AUD_Error gmmhmm_export( void *hGmmHmm, AUD_Int8s *pFileName );

#endif // _AUD_RECOG_H
