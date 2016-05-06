/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
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


#include "mfcc16s.h"
#include "spectral.h"

typedef struct
{
	Fft_16s         *hFft;
	AUD_Window16s   *hWin;

	AUD_Int32s      winLen;
	AUD_Int32s      frameStride;
	AUD_AmpCompress compressType;

	AUD_Int16s      *pWorkBuffer;
} _AUD_Spectrogram16s32sState;

typedef struct
{
	void            *hSpecHandle;
	AUD_Int32s      winLen;
	AUD_Int32s      frameStride;
	AUD_AmpCompress compressType;
} _AUD_Tfi16s32sState;

typedef struct
{
	void            *hSpecHandle;
	
	Mfcc            *hMfccHandle;
	Mfcc_rasta      *hMfccRastaHandle;
	Mfcc_delta      *hMfccDeltaHandle;
	Mfcc_fdelta     *hMfccFDeltaHandle;
	Mfcc_deltadelta *hMfccDeltaDeltaHandle;

#if SPECFEAT_LEN
	SpecFeat        *hSpecFeatHandle;
#endif

	AUD_Int32s      winLen;
	AUD_Int32s      sampleRate;
	AUD_Int32s      frameStride;
	AUD_Int16s      featureOrder;
	AUD_AmpCompress compressType;
	AUD_Int32s      delayNum;
	AUD_Int32s      delaySpec;
} _AUD_Mfcc16s32sState;

// mfcc
AUD_Error mfcc16s32s_init( void **phMfccHandle, 
                           AUD_Int32s winLen, AUD_WindowType winType, AUD_Int16s featureOrder, AUD_Int32s frameStride, AUD_Int32s sampleRate, AUD_AmpCompress compressType )
{
	AUD_ASSERT( (winLen > 0) && (featureOrder > 0) && (frameStride > 0) && (sampleRate > 0) );

	AUD_Error error = AUD_ERROR_NONE;

	_AUD_Mfcc16s32sState *pState = (_AUD_Mfcc16s32sState*)calloc( sizeof(_AUD_Mfcc16s32sState), 1 );
	if ( pState == NULL )
	{
		*phMfccHandle = NULL;
		return AUD_ERROR_OUTOFMEMORY;
	}

	// init spectrogram

	error = spectrogram16s32s_init( &(pState->hSpecHandle), winLen, winType, frameStride, AUD_AMPCOMPRESS_NONE );
	AUD_ASSERT( pState->hSpecHandle );

	// init mfcc related handles

	mfcc_init( &(pState->hMfccHandle), winLen, sampleRate, MELFB_LOW, MELFB_HIGH, MELFB_ORDER, 1127.0, 700.0, featureOrder, compressType );

#if RASTA_MFCC
	mfccrasta_init( &(pState->hMfccRastaHandle), RASTA_MFCC );
#endif

#if DELTA_MFCC
	mfccd_init( &(pState->hMfccDeltaHandle), DELTA_MFCC );
#endif

#if DDELTA_MFCC
	mfccdd_init( &(pState->hMfccDeltaDeltaHandle), DDELTA_MFCC );
#endif

#if FDELTA_MFCC
	mfccfd_init( &(pState->hMfccFDeltaHandle), FDELTA_MFCC );
#endif

#if SPECFEAT_LEN
	specfeat_init( &(pState->hSpecFeatHandle), winLen, sampleRate );
#endif

	pState->frameStride  = frameStride;
	pState->winLen       = winLen;
	pState->featureOrder = featureOrder;
	pState->sampleRate   = sampleRate;
	pState->compressType = compressType;

	pState->delaySpec    = MFCC_DELAY;
	pState->delayNum     = 0;

	*phMfccHandle        = pState;

	return error;
}

AUD_Error mfcc16s32s_deinit( void **phMfccHandle )
{
	AUD_ASSERT( phMfccHandle != NULL && *phMfccHandle != NULL );

	AUD_Error error = AUD_ERROR_NONE;

	_AUD_Mfcc16s32sState *pState = (_AUD_Mfcc16s32sState*)(*phMfccHandle);

	error = spectrogram16s32s_deinit( &(pState->hSpecHandle) );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	mfcc_free( &(pState->hMfccHandle) );

#if RASTA_MFCC
	mfccrasta_free( &(pState->hMfccRastaHandle) );
#endif

#if DELTA_MFCC
	mfccd_free( &(pState->hMfccDeltaHandle) );
#endif

#if DDELTA_MFCC
	mfccdd_free( &(pState->hMfccDeltaDeltaHandle) );
#endif

#if FDELTA_MFCC
	mfccfd_free( &(pState->hMfccFDeltaHandle) );
#endif

#if SPECFEAT_LEN
	specfeat_free( &(pState->hSpecFeatHandle) );
#endif

	free( pState );
	pState = NULL;

	*phMfccHandle = NULL;

	return error;
}

AUD_Error mfcc16s32s_refresh( void *hMfccHandle )
{
	_AUD_Mfcc16s32sState *pState = (_AUD_Mfcc16s32sState*)hMfccHandle;

	pState->delayNum = 0;

#if RASTA_MFCC
	mfccrasta_refresh( pState->hMfccRastaHandle );
#endif

#if DELTA_MFCC
	mfccd_refresh( pState->hMfccDeltaHandle );
#endif

#if DDELTA_MFCC
	mfccdd_refresh( pState->hMfccDeltaDeltaHandle );
#endif

	return AUD_ERROR_NONE;
}

AUD_Error mfcc16s32s_calc( void *hMfccHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                           AUD_Feature *pOutMFCC )
{
	AUD_ASSERT( hMfccHandle != NULL && pInBuffer != NULL && pOutMFCC != NULL && sampleNum > 0 );

	AUD_Int32s i = 0, j = 0, m = 0;
	AUD_Error  error = AUD_ERROR_NONE;

	_AUD_Mfcc16s32sState *pState = (_AUD_Mfcc16s32sState*)hMfccHandle;

	// get spectrogram
	AUD_Bool      isRewind = AUD_FALSE;
	AUD_Int32s    outTimeSlices = 0;
	AUD_Int32s    ret;

	AUD_Matrix outSpectrogram;
	for ( i = 0; i * pState->frameStride + pState->winLen <= sampleNum; i++ )
	{
		;
	}

	outSpectrogram.rows     = i;
	outSpectrogram.cols     = pState->winLen;
	outSpectrogram.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &outSpectrogram );
	AUD_ASSERT( ret == 0 );

	// AUDLOG( "total %d time strides\n", i );

	error = spectrogram16s32s_calc( pState->hSpecHandle, pInBuffer, sampleNum,
	                                &outSpectrogram, &outTimeSlices, &isRewind );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUD_Int32s startRow = ( outTimeSlices <= outSpectrogram.rows ) ?  0 : ( outTimeSlices % outSpectrogram.rows );

	AUD_Int32s *pTmpBuf = (AUD_Int32s*)malloc( (pState->featureOrder + 1) * sizeof(AUD_Int32s) );
	AUD_ASSERT( pTmpBuf );

	// discard the first delaySpec time slices feature
	m = 0;
	while ( ( pState->delayNum < pState->delaySpec ) && ( m < outSpectrogram.rows ) )
	{
		AUD_Int32s *pDummyMfcc = pOutMFCC->featureMatrix.pInt32s;
		AUD_Int32s *pFFTRow    = outSpectrogram.pInt32s + startRow * outSpectrogram.cols;

		mfcc_calc( pState->hMfccHandle, pFFTRow, pTmpBuf );

#if LOG_ENERGY
		AUD_Double energy = 0.;
		for ( j = 0; j < outSpectrogram.cols; j++ )
		{
			energy += pow( log( pFFTRow[j] / 32768. ), 2 );
		}
		energy = ( energy > exp( -50. ) ) ? energy : exp( -50. );
		energy = sqrt( log( energy ) ) * 32768;
		pTmpBuf[pState->featureOrder] = (AUD_Int32s)round( energy );
#endif

#if RAW_MFCC
		mfcc_get( pState->hMfccHandle, pDummyMfcc );
#endif

#if RASTA_MFCC
		mfccrasta_calc( pState->hMfccRastaHandle, pTmpBuf, pDummyMfcc + RAW_MFCC );
#endif

#if DELTA_MFCC
		mfccd_calc( pState->hMfccDeltaHandle, pTmpBuf, pDummyMfcc + RAW_MFCC + RASTA_MFCC );
#endif

#if DDELTA_MFCC
		mfccdd_calc( pState->hMfccDeltaDeltaHandle, pTmpBuf, pDummyMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC );
#endif

#if FDELTA_MFCC
		mfccfd_calc( pState->hMfccFDeltaHandle, pTmpBuf, pDummyMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC + DDELTA_MFCC );
#endif

#if SPECFEAT_LEN
		specfeat_calc( pState->hSpecFeatHandle, pFFTRow, pDummyMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC + DDELTA_MFCC + FDELTA_MFCC );
#endif

		startRow = ( startRow + 1 ) % outSpectrogram.rows;
		(pState->delayNum)++;
		m++;
	}
	if ( pState->delayNum < pState->delaySpec || m >= outSpectrogram.rows )
	{
		error = AUD_ERROR_MOREDATA;
		if ( pTmpBuf )
		{
			free( pTmpBuf );
			pTmpBuf = NULL;
		}
		ret = destroyMatrix( &outSpectrogram );
		AUD_ASSERT( ret == 0 );

		return error;
	}

	AUD_Int32s rowLimit = AUD_MIN( pOutMFCC->featureMatrix.rows, AUD_MIN( outTimeSlices - m, outSpectrogram.rows - m ) );
	m = 0;
	for ( i = startRow; ; i = (i + 1) % outSpectrogram.rows )
	{
		AUD_Int32s *pCurrentMfcc = pOutMFCC->featureMatrix.pInt32s + m * pOutMFCC->featureMatrix.cols;
		AUD_Int32s *pFFTRow      = outSpectrogram.pInt32s + i * outSpectrogram.cols;

		// calc mfcc
		mfcc_calc( pState->hMfccHandle, pFFTRow, pTmpBuf );

#if LOG_ENERGY
		AUD_Double energy = 0.;
		for ( j = 0; j < outSpectrogram.cols; j++ )
		{
			energy += pow( log( pFFTRow[j] / 32768. ), 2 );
		}
		energy = ( energy > exp( -50. ) ) ? energy : exp( -50. );
		energy = sqrt( log( energy ) ) * 32768;
		pTmpBuf[pState->featureOrder] = (AUD_Int32s)round( energy );
#endif

#if RAW_MFCC
		mfcc_get( pState->hMfccHandle, pCurrentMfcc );
#endif

#if RASTA_MFCC
		mfccrasta_calc( pState->hMfccRastaHandle, pTmpBuf, pCurrentMfcc + RAW_MFCC );
#endif

#if DELTA_MFCC
		mfccd_calc( pState->hMfccDeltaHandle, pTmpBuf, pCurrentMfcc + RAW_MFCC + RASTA_MFCC );
#endif

#if DDELTA_MFCC
		mfccdd_calc( pState->hMfccDeltaDeltaHandle, pTmpBuf, pCurrentMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC );
#endif

#if FDELTA_MFCC
		mfccfd_calc( pState->hMfccFDeltaHandle, pTmpBuf, pCurrentMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC + DDELTA_MFCC );
#endif

#if SPECFEAT_LEN
		specfeat_calc( pState->hSpecFeatHandle, pFFTRow, pCurrentMfcc + RAW_MFCC + RASTA_MFCC + DELTA_MFCC + DDELTA_MFCC + FDELTA_MFCC );
#endif

		m++;
		if ( m >= rowLimit )
		{
			break;
		}
	}

#if ENABLE_MVN
	ret = mvnMatrix( &(pOutMFCC->featureMatrix), 2 );
	AUD_ASSERT( ret == 0 );
#endif

	// calc norm for each feature vector
	AUD_Double tmp;
	for ( i = 0; i < pOutMFCC->featureMatrix.rows; i++ )
	{
		(pOutMFCC->featureNorm.pInt64s)[i] = 0;
		tmp = 0.;
		for ( j = 0; j < pOutMFCC->featureMatrix.cols; j++ )
		{
			tmp += (AUD_Double)( pOutMFCC->featureMatrix.pInt32s[ i * pOutMFCC->featureMatrix.cols + j] ) *
			        pOutMFCC->featureMatrix.pInt32s[ i * pOutMFCC->featureMatrix.cols + j];
		}
		(pOutMFCC->featureNorm.pInt64s)[i] = (AUD_Int64s)sqrt( tmp );
	}

#if 0
	AUDLOG( "feature matrix\n" );
	for ( i = 0; i < pOutMFCC->featureMatrix.rows; i++ )
	{
		for ( j = 0; j < pOutMFCC->featureMatrix.cols; j++ )
		{
			AUDLOG( "%d, ", *(pOutMFCC->featureMatrix.pInt32s + i * pOutMFCC->featureMatrix.cols + j) );
		}
		AUDLOG( "norm: %lld\n", pOutMFCC->featureNorm.pInt64s[i] );
	}
	AUDLOG( "\n\n" );
#endif

	if ( pTmpBuf )
	{
		free( pTmpBuf );
		pTmpBuf = NULL;
	}
	ret = destroyMatrix( &outSpectrogram );
	AUD_ASSERT( ret == 0 );

	return error;
}


// time-frequency image
AUD_Error tfi16s32s_init( void **phTfiHandle,
                          AUD_Int32s winLen, AUD_WindowType winType, AUD_Int32s frameStride, AUD_AmpCompress compressType )
{
	AUD_Error error = AUD_ERROR_NONE;

	_AUD_Tfi16s32sState *pState = (_AUD_Tfi16s32sState*)calloc( sizeof(_AUD_Tfi16s32sState), 1 );
	if ( pState == NULL )
	{
		*phTfiHandle = NULL;
		return AUD_ERROR_OUTOFMEMORY;
	}

	error = spectrogram16s32s_init( &(pState->hSpecHandle), winLen, winType, frameStride, compressType );
	AUD_ASSERT( pState->hSpecHandle );

	pState->frameStride    = frameStride;
	pState->winLen         = winLen;
	pState->compressType   = compressType;

	*phTfiHandle = pState;

	return error;
}

AUD_Error tfi16s32s_deinit( void **phTfiHandle )
{
	AUD_ASSERT( phTfiHandle != NULL && *phTfiHandle != NULL );

	AUD_Error error = AUD_ERROR_NONE;

	_AUD_Tfi16s32sState *pState = (_AUD_Tfi16s32sState*)(*phTfiHandle);

	error = spectrogram16s32s_deinit( &(pState->hSpecHandle) );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	free( pState );

	*phTfiHandle = NULL;

	return error;
}

AUD_Error tfi16s32s_calc( void *hTfiHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                          AUD_Feature *pOutTFI )
{
	AUD_ASSERT( hTfiHandle != NULL && pInBuffer != NULL && pOutTFI != NULL && sampleNum > 0 );

	AUD_Int32s i = 0, j = 0, m = 0, n = 0;
	AUD_Error  error = AUD_ERROR_NONE;

	_AUD_Tfi16s32sState *pState = (_AUD_Tfi16s32sState*)hTfiHandle;

	// get spectrogram
	AUD_Bool   isRewind      = AUD_FALSE;
	AUD_Int32s outTimeSlices = 0, ret;

	AUD_Matrix outSpectrogram;

	// calc window number
	for ( i = 0; ( i * pState->frameStride + pState->winLen ) <= sampleNum; i++ )
	{
		;
	}

	if ( i < 3 )
	{
		return AUD_ERROR_MOREDATA;
	}

	outSpectrogram.rows     = i;
	outSpectrogram.cols     = pState->winLen;
	outSpectrogram.dataType = AUD_DATATYPE_INT32S;
	ret = createMatrix( &outSpectrogram );
	AUD_ASSERT( ret == 0 );

	error = spectrogram16s32s_calc( pState->hSpecHandle, pInBuffer, sampleNum,
	                                &outSpectrogram, &outTimeSlices, &isRewind );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUD_Int32s startRow = ( outTimeSlices <= outSpectrogram.rows ) ? 1 : ((outTimeSlices + 1) % outSpectrogram.rows);
	AUD_Int32s rowLimit = AUD_MIN( pOutTFI->featureMatrix.rows, AUD_MIN( outTimeSlices - 2, outSpectrogram.rows - 2 ) );
	AUD_Int32s mag, magTPrev, magTNext, magFPrev, magFNext;
	AUD_Int32s derivT, derivF;

	m = 0;
	for ( i = startRow; ; i = (i + 1) % outSpectrogram.rows )
	{
		AUD_Double  tmp = 0;

		n = 0;
		(pOutTFI->featureNorm.pInt64s)[m] = 0;
		for ( j = 1; j < outSpectrogram.cols / 2 - 1; j++ )
		{
			mag      = *(outSpectrogram.pInt32s + i * outSpectrogram.cols + j);
			magTPrev = *(outSpectrogram.pInt32s + AUD_CIRPREV( i, outSpectrogram.rows ) * outSpectrogram.cols + j);
			magTNext = *(outSpectrogram.pInt32s + AUD_CIRNEXT( i, outSpectrogram.rows ) * outSpectrogram.cols + j);
			derivT   = (AUD_Int32s)( ( magTPrev + (AUD_Int64s)magTNext - ( (AUD_Int64s)mag << 1 ) ) / 2 ); // modify derivT formula here

			magFPrev = *(outSpectrogram.pInt32s + i * outSpectrogram.cols + j - 1 );
			magFNext = *(outSpectrogram.pInt32s + i * outSpectrogram.cols + j + 1 );
			derivF   = (AUD_Int32s)( ( magFPrev + (AUD_Int64s)magFNext - ( (AUD_Int64s)magFPrev << 1 ) ) / 2 );

			*(pOutTFI->featureMatrix.pInt32s + m * pOutTFI->featureMatrix.cols + n)                                   = derivT;
			*(pOutTFI->featureMatrix.pInt32s + m * pOutTFI->featureMatrix.cols + n + pOutTFI->featureMatrix.cols / 2) = derivF;

			tmp += (AUD_Double)derivT * derivT + (AUD_Double)derivF * derivF;
			n++;
		}
		(pOutTFI->featureNorm.pInt64s)[m] = (AUD_Int64s)sqrt( tmp );

		if ( ++m >= rowLimit )
		{
			break;
		}
	}

#if 0
	AUDLOG( "feature matrix\n" );
	for ( i = 0; i < m; i++ )
	{
		for ( j = 0; j < pOutTFI->featureMatrix.cols; j++ )
		{
			AUDLOG( "%d, ", *(pOutTFI->featureMatrix.pInt32s + i * pOutTFI->featureMatrix.cols + j) );
		}
		AUDLOG( "len: %lld\n", pOutTFI->pFeatureNorm[i] );
	}
	AUDLOG( "\n\n" );
#endif

	ret = destroyMatrix( &outSpectrogram );
	AUD_ASSERT( ret == 0 );

	return AUD_ERROR_NONE;
}



// spectrogram
AUD_Error spectrogram16s32s_init( void **phSpecHandle, AUD_Int32s winLen, AUD_WindowType winType, AUD_Int32s frameStride, AUD_AmpCompress compressType )
{
	_AUD_Spectrogram16s32sState *pState = (_AUD_Spectrogram16s32sState*)calloc( sizeof(_AUD_Spectrogram16s32sState), 1 );
	if ( pState == NULL )
	{
		*phSpecHandle = NULL;
		return AUD_ERROR_OUTOFMEMORY;
	}

	// init fft module
	fft_init( &(pState->hFft), winLen, 15 );
	AUD_ASSERT( pState->hFft );

	// init hanning module
	win16s_init( &(pState->hWin), winType, winLen, 14 );
	AUD_ASSERT( pState->hWin );

	pState->frameStride    = frameStride;
	pState->winLen         = winLen;
	pState->compressType   = compressType;

	// allocate memory for working buffer
	pState->pWorkBuffer = (AUD_Int16s*)malloc( winLen * sizeof(AUD_Int16s) );
	AUD_ASSERT( pState->pWorkBuffer );

	*phSpecHandle = pState;

	return AUD_ERROR_NONE;
}

AUD_Error spectrogram16s32s_deinit( void **phSpecHandle )
{
	AUD_ASSERT( phSpecHandle != NULL && *phSpecHandle != NULL );

	_AUD_Spectrogram16s32sState *pState = (_AUD_Spectrogram16s32sState*)(*phSpecHandle);

	fft_free( &(pState->hFft) );
	win16s_free( &(pState->hWin) );
	free( pState->pWorkBuffer );

	free( pState );

	*phSpecHandle = NULL;

	return AUD_ERROR_NONE;
}

AUD_Error spectrogram16s32s_calc( void *hSpecHandle, AUD_Int16s *pInBuffer, AUD_Int32s sampleNum,
                                  AUD_Matrix *pOutSpectrogram, AUD_Int32s *pOutTimeSlices, AUD_Bool *pIsRewind )
{
	AUD_ASSERT( hSpecHandle != NULL && pInBuffer != NULL && pOutSpectrogram != NULL && pOutTimeSlices != NULL && pIsRewind != NULL );

	AUD_Int32s                  i, j;
	AUD_Int32s                  allocTimeSlices = pOutSpectrogram->rows;
	_AUD_Spectrogram16s32sState *pState         = (_AUD_Spectrogram16s32sState*)hSpecHandle;

	*pOutTimeSlices = 0;
	*pIsRewind      = AUD_FALSE;

	// get spectral energy matrix based on timeSlices
	for ( i = 0; (*pOutTimeSlices) * pState->frameStride + pState->winLen <= sampleNum; i = (i + 1) % allocTimeSlices )
	{
		AUD_Int16s *pBuf            = pInBuffer + (*pOutTimeSlices) * pState->frameStride;
		AUD_Int32s *pSpectrogramRow = pOutSpectrogram->pInt32s + i * pOutSpectrogram->cols;

#if 1
		sig_normalize( pBuf, pState->pWorkBuffer, pState->winLen );

		win16s_calc( pState->hWin, pState->pWorkBuffer, pState->pWorkBuffer );
#else
		win16s_calc( pState->hWin, pBuf, pState->pWorkBuffer );
#endif

		fft_mag( pState->hFft, pState->pWorkBuffer, pState->winLen, pSpectrogramRow );

		// take advantage of DFT's "conjungate symmetry"
		if ( pState->compressType == AUD_AMPCOMPRESS_LOG )
		{
			for ( j = 0; j < pOutSpectrogram->cols / 2; j++ )
			{
				pSpectrogramRow[j] = (AUD_Int32s)( log10( (AUD_Double)pSpectrogramRow[j + 1] ) * 20. );
			}
		}
		else if ( pState->compressType == AUD_AMPCOMPRESS_ROOT )
		{
			for ( j = 0; j < pOutSpectrogram->cols / 2; j++ )
			{
				pSpectrogramRow[j] = (AUD_Int32s)( pow( (AUD_Double)pSpectrogramRow[j + 1], 0.34 ) );
			}
		}

		(*pOutTimeSlices)++;
	}

	if ( *pOutTimeSlices > allocTimeSlices )
	{
		AUDLOG( "!!! REWIND happened\n" );
		*pIsRewind = AUD_TRUE;
	}

	return AUD_ERROR_NONE;	
}

