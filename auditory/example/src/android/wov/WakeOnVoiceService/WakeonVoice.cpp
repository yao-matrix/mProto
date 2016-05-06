/***************************************************************
* Intel MCG PSI Core TR project: mAudio, where m means magic
*   Author: Yao, Matrix( matrix.yao@intel.com )
* Copyright(c) 2012 Intel Corporation
* ALL RIGHTS RESERVED
***************************************************************/
#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <cutils/properties.h>

#include "IWakeonVoice.h"
#include "WakeonVoice.h"
#include "WakeonVoiceClient.h"

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioConfig.h"
#include "AudioUtil.h"

#include "io/streaming/android/AlsaRecorder.h"
#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"
#include "math/mmath.h"

#undef LOG_TAG
#define LOG_TAG "WakeOnVoice_NativeServer"
#include <utils/Log.h>

#define PROP_THRESH "service.wov.threshold"
#define PROP_LOG    "service.wov.log"

using namespace android;

#define PERF_PROFILE   0


#define MAX_ENROLL_SAMPLE_NUM  (SAMPLE_RATE * 4)
#define KEYWORD_FEAT           "/sdcard/wov/keyword/keyword.feat"

namespace wakeonvoice
{
	static sp<IWakeonVoiceClient> client;
	static CAlsaRecorder          *alsaRecorder;

	static char                   *pEnrollBuf;
	static int                    enrollBufLen;
	static char                   *pStreamBuf;
	static int                    streamBufLen;
	static char                   *pFrameBuf;
	static int                    frameBufLen;
	static char                   *pCleanBuf;
	static int                    cleanLen;

	// template file
	static char                   keywordFile[512] = KEYWORD_FEAT;
	static AUD_Feature            keywordFeat;

	static double                 threshold;
	static void                   *hVadHandle      = NULL;

	static int                    minLen           = 0;

	static int                    listenRefCnt     = 0;

	static int                    enrollNum        = 0;
	static int                    maxEnrollNum     = 1;

	static AUD_Feature            *pEnrollFeat     = NULL;

	static int                    enableDebug      = 0;
	AUD_Tick                      t;

	static status_t (*fnWorker)( int, int );

	status_t isValidTemplate( short *pBuf, int validSampleNum )
	{
		return 1;

		if ( validSampleNum < 18000 )
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	void fnFrameAdmissionCallback()
	{
		while ( 1 )
		{
			AUD_Error error          = AUD_ERROR_NONE;
			status_t  ret            = 0;
			int       validSampleNum = 0;

			ret = alsaRecorder->fnGetFrame( pFrameBuf, frameBufLen );
			if ( ret != OK )
			{
				return;
			}

			// TODO: frame admission, if validSampleNum > 0, means frame admission success and ready to recognition, and validSampleNum is the valid samples in buffer
			int start = 0;
// #if PERF_PROFILE
			// t = -time_gettime();
// #endif
			validSampleNum = sig_vadupdate( hVadHandle, (short*)pFrameBuf, &start );
// #if PERF_PROFILE
			// t += time_gettime();
			// LOGD( "===========PERF============fnFrameAdmissionCallback---sig_vadupdate cost %.2f ms ===========\n", t / 1000. );
// #endif

			if ( validSampleNum > 0 )
			{
				LOGE( "#####Found suspect speech with length: %d\n", validSampleNum );

				ret = fnWorker( validSampleNum, start );
				if ( ret == 0 )
				{
					alsaRecorder->fnClean();
					return;
				}
			}
		}

		return;
	}

	status_t fnKeywordDetection( int validSampleNum, int start )
	{
		// Mutex::Autolock _l( mLock );

		int       j     = 0;
		int       ret;
		AUD_Error error = AUD_ERROR_NONE;

		if ( validSampleNum / FRAME_STRIDE < minLen )
		{
			LOGE( "discard for too short" );
			return 1;
		}

		// pCleanBuf = (char*)calloc( ( validSampleNum + start ) * BYTES_PER_SAMPLE, 1 );
		// AUD_ASSERT( pCleanBuf );

		// Pre-processing: pre-emphasis
		sig_preemphasis( (short*)pStreamBuf, (short*)pStreamBuf, ( validSampleNum + start ) );

		// De-noise
#if PERF_PROFILE
		t = -time_gettime();
#endif
		// cleanLen = denoise_mmse( (short*)pStreamBuf, (short*)pCleanBuf, (validSampleNum + start) );
		// cleanLen = denoise_wiener( (short*)pStreamBuf, (short*)pCleanBuf, (validSampleNum + start) );
#if PERF_PROFILE
		t += time_gettime();
		LOGD( "===========PERF============fnFrameAdmissionCallback---denoise cost %.2f ms ===========\n", t / 1000. );
#endif

		// create feature matrix
		int inFrameNum = 0;
		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= validSampleNum; j++ )
		{
			inFrameNum++;
		}

		AUD_Feature inFeature;
		inFeature.featureMatrix.rows     = inFrameNum - MFCC_DELAY;
		inFeature.featureMatrix.cols     = MFCC_FEATDIM;
		inFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		inFeature.featureNorm.len      = inFeature.featureMatrix.rows;
		inFeature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		double          score;
		void            *hFeatHandle = NULL;
		AUD_DTWSession  dtwSession;

		// init mfcc handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc MFCC feature
		// error = mfcc16s32s_calc( hFeatHandle, (short*)pCleanBuf, cleanLen, &inFeature );

		error = mfcc16s32s_calc( hFeatHandle, (short*)(pStreamBuf + start), validSampleNum, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// init dtw match
		dtwSession.dtwType        = WOV_DTW_TYPE;
		dtwSession.distType       = WOV_DISTANCE_METRIC;
		dtwSession.transitionType = WOV_DTWTRANSITION_STYLE;
		error = dtw_initsession( &dtwSession, &keywordFeat, inFeature.featureMatrix.rows );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = dtw_updatefrmcost( &dtwSession, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = dtw_match( &dtwSession, WOV_DTWSCORING_METHOD, &score, NULL );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// char value[PROPERTY_VALUE_MAX];
		// property_get( PROP_THRESH, value, WOV_THRESHOLD );
		// LOGD( "threshold: %s", value );
		/*
		if ( threshold != atof( value ) )
		{
			threshold = atof( value );
			LOGD( "threshold changed to: %.2f", threshold );
		}
		*/
		LOGD( "#####utterance with score: %.2f\n", score );

		if ( enableDebug == 1 )
		{
			char          fileName[512] = { 0, };
			unsigned int  t             = (unsigned int)( systemTime() / 1000000 );
			sprintf( fileName, "/sdcard/wov/debug/%u_%.2f.wav", t, score );
			ret = writeWavToFile( (AUD_Int8s*)fileName, (short*)pStreamBuf, validSampleNum + start );
			AUD_ASSERT( ret == 0 );
			LOGD( "==================debug file write to %s===================\n", fileName );
		}

		if ( score <= threshold )
		{
#if 1
			// int t = systemTime();
			// LOGE( "#####BEFORE CALLBACK\n" );
			client->notifyCallback( WAKE_ON_KEYWORD, 1, 1 );
			// t = systemTime() - t;
			// LOGE( "#####notify callback time: %.2f ms\n", t / 1000000.0 );
#endif
		}

		error = dtw_deinitsession( &dtwSession );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// free( pCleanBuf );
		// pCleanBuf = NULL;

		return 0;
	}

	status_t fnFuseTemplate()
	{
		// multi-enroll fuse template here
		int       i = 0, j = 0, k = 0;
		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

		AUD_Double **ppQuality = NULL;
		AUD_Vector **ppPath    = NULL;

		ppQuality = (AUD_Double**)calloc( maxEnrollNum * sizeof(AUD_Double*), 1 );
		for ( i = 0; i < maxEnrollNum; i++ )
		{
			ppQuality[i] = (AUD_Double*)calloc( maxEnrollNum * sizeof(AUD_Double), 1 );
		}

		ppPath = (AUD_Vector**)calloc( maxEnrollNum * sizeof(AUD_Vector*), 1 );
		for ( i = 0; i < maxEnrollNum; i++ )
		{
			ppPath[i] = (AUD_Vector*)calloc( maxEnrollNum * sizeof(AUD_Vector), 1 );
		}

		for ( i = 0; i < maxEnrollNum; i++ )
		{
			for ( j = 0; j < maxEnrollNum; j++ )
			{
				if ( j == i )
				{
					ppQuality[i][j]      = 0.;
					ppPath[i][j].pInt32s = NULL;
					continue;
				}

				ppPath[i][j].len      = pEnrollFeat[i].featureMatrix.rows;
				ppPath[i][j].dataType = AUD_DATATYPE_INT32S;
				ret = createVector( &(ppPath[i][j]) );
				AUD_ASSERT( ret == 0 );

				for ( int m = 0; m < ppPath[i][j].len; m++ )
				{
					(ppPath[i][j].pInt32s)[m] = -1;
				}

				AUD_DTWSession  dtwSession;

				dtwSession.dtwType        = WOV_DTW_TYPE;
				dtwSession.distType       = WOV_DISTANCE_METRIC;
				dtwSession.transitionType = WOV_DTWTRANSITION_STYLE;
				error = dtw_initsession( &dtwSession, &(pEnrollFeat[i]), pEnrollFeat[j].featureMatrix.rows );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// update frame cost
				error = dtw_updatefrmcost( &dtwSession, &(pEnrollFeat[j]) );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				// compute DTW
				error = dtw_match( &dtwSession, WOV_DTWSCORING_METHOD, &(ppQuality[i][j]), &(ppPath[i][j]) );
				AUD_ASSERT( error == AUD_ERROR_NONE );

				error = dtw_deinitsession( &dtwSession );
				AUD_ASSERT( error == AUD_ERROR_NONE );
			}
		}

		AUD_Double bestQuality  = 0.;
		for ( j = 0; j < maxEnrollNum; j++ )
		{
			// AUDLOG( "%.3f, ", ppQuality[0][j] );
			bestQuality += ppQuality[0][j];
		}
		AUD_Int32s bestTemplate = 0;

		for ( i = 1; i < maxEnrollNum; i++ )
		{
			AUD_Double quality = 0.;

			for ( j = 0; j < maxEnrollNum; j++ )
			{
				// AUDLOG( "%.3f, ", ppQuality[i][j] );
				quality += ppQuality[i][j];
			}

			if ( quality < bestQuality )
			{
				bestQuality  = quality;
				bestTemplate = i;
			}
		}

		keywordFeat.featureMatrix.rows     = pEnrollFeat[bestTemplate].featureMatrix.rows;
		keywordFeat.featureMatrix.cols     = MFCC_FEATDIM;
		keywordFeat.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(keywordFeat.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		keywordFeat.featureNorm.len       = pEnrollFeat[bestTemplate].featureMatrix.rows;
		keywordFeat.featureNorm.dataType  = AUD_DATATYPE_INT64S;
		ret = createVector( &(keywordFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		AUD_Matrix *pMainMatrix    = &(pEnrollFeat[bestTemplate].featureMatrix);
		AUD_Matrix **ppAssistMatrix = (AUD_Matrix**)calloc( (maxEnrollNum - 1) * sizeof( AUD_Matrix* ), 1 );
		AUD_ASSERT( ppAssistMatrix );
		AUD_Int32s **ppAssistPath = (AUD_Int32s**)calloc( (maxEnrollNum - 1) * sizeof( AUD_Int32s* ), 1 );
		AUD_ASSERT( ppAssistPath );

		for ( i = 0; i < maxEnrollNum - 1; i++ )
		{
			ppAssistMatrix[i] = &(pEnrollFeat[(bestTemplate + i + 1) % maxEnrollNum].featureMatrix);
			ppAssistPath[i]   = ppPath[bestTemplate][(bestTemplate + i + 1) % maxEnrollNum].pInt32s;
		}

		// weight determination
		AUD_Double *pWeight = (AUD_Double*)calloc( maxEnrollNum * sizeof(AUD_Double), 1 );
		AUD_ASSERT( pWeight );

		pWeight[maxEnrollNum - 1] = 2.;
		AUD_Double sum = pWeight[maxEnrollNum - 1];
		for ( j = 0; j < maxEnrollNum - 1; j++ )
		{
			pWeight[j] = 2. - ppQuality[bestTemplate][(bestTemplate + j + 1) % maxEnrollNum];
			sum += pWeight[j];
		}

		for ( j = 0; j < maxEnrollNum; j++ )
		{
			pWeight[j] /= sum;
		}

		AUD_Int32s **ppAssistFeat = (AUD_Int32s**)calloc( (maxEnrollNum - 1) * sizeof( AUD_Int32s* ), 1 );
		AUD_ASSERT( ppAssistFeat );

		for ( i = 0; i < keywordFeat.featureMatrix.rows; i++ )
		{
			AUD_Int32s *pFused = keywordFeat.featureMatrix.pInt32s + i * keywordFeat.featureMatrix.cols;

			AUD_Int32s *pMainFeat = pMainMatrix->pInt32s + i * pMainMatrix->cols;

			for ( j = 0; j < maxEnrollNum - 1; j++ )
			{
				if ( ppAssistPath[j][i] != -1 )
				{
					ppAssistFeat[j] = ppAssistMatrix[j]->pInt32s + ppAssistPath[j][i] * ppAssistMatrix[j]->cols;
				}
				else
				{
					ppAssistFeat[j] = pMainMatrix->pInt32s + i * pMainMatrix->cols;
				}
			}

			AUD_Double tmp = 0.;
			for ( j = 0; j < keywordFeat.featureMatrix.cols; j++ )
			{
				AUD_Double val = 0.;
				for ( k = 0; k < maxEnrollNum - 1; k++ )
				{
					val += pWeight[k] * ppAssistFeat[k][j];
				}
				val += pMainFeat[j] * pWeight[maxEnrollNum - 1];
				pFused[j] = (AUD_Int32s)round( val );

				tmp += val * val;
			}
			(keywordFeat.featureNorm.pInt64s)[i] = (AUD_Int64s)sqrt( tmp );
		}

		ret = writeMatrixToFile( &(keywordFeat.featureMatrix), (AUD_Int8s*)keywordFile );
		AUD_ASSERT( ret == 0 );

		free( pWeight );
		pWeight = NULL;

		free( ppAssistMatrix );
		ppAssistMatrix = NULL;
		free( ppAssistPath );
		ppAssistPath = NULL;

		free( ppAssistFeat );
		ppAssistFeat = NULL;

		for ( i = 0; i < maxEnrollNum; i++ )
		{
			for ( j = 0; j < maxEnrollNum; j++ )
			{
				if ( i != j )
				{
					ret = destroyVector( &(ppPath[i][j]) );
					AUD_ASSERT( ret == 0 );
				}
			}
		}

		for ( i = 0; i < maxEnrollNum; i++ )
		{
			free( ppQuality[i] );
			free( ppPath[i] );
		}
		free( ppQuality );
		free( ppPath );

		ret = destroyMatrix( &(keywordFeat.featureMatrix) );
		AUD_ASSERT( ret == 0 );
		ret = destroyVector( &(keywordFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		return 0;
	}

	status_t fnExtractTemplate( int validSampleNum, int start )
	{
		// Mutex::Autolock _l( mLock );
		alsaRecorder->fnStop();
		alsaRecorder->fnRegisterCallback( NULL );

		if ( enrollNum >= maxEnrollNum )
		{
			LOGE( "enroll times{%d} exceed limit[%d]\n", enrollNum, maxEnrollNum );
			return -1;
		}

		// because it need smooth strategy to enroll two words together, tail of end is not real speech. Give up 0.16s.
		// validSampleNum = validSampleNum - 0.16 * SAMPLE_RATE;

		// check whether valid template
		int validCheck;
		validCheck = isValidTemplate( (short*)pEnrollBuf + start, validSampleNum );
		if ( validCheck == 0 )
		{
			LOGE( "======================= too weak =======================\n" );
			client->notifyCallback( ENROLL_FAIL, 0, 1 );
			return 0;
		}

		int       j = 0;
		int       ret;
		AUD_Error error = AUD_ERROR_NONE;

		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= validSampleNum; j++ )
		{
			;
		}

		pEnrollFeat[enrollNum].featureMatrix.rows     = j - MFCC_DELAY;
		pEnrollFeat[enrollNum].featureMatrix.cols     = MFCC_FEATDIM;
		pEnrollFeat[enrollNum].featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(pEnrollFeat[enrollNum].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		pEnrollFeat[enrollNum].featureNorm.len        = pEnrollFeat[enrollNum].featureMatrix.rows;
		pEnrollFeat[enrollNum].featureNorm.dataType   = AUD_DATATYPE_INT64S;
		ret = createVector( &(pEnrollFeat[enrollNum].featureNorm) );
		AUD_ASSERT( ret == 0 );

		// Pre-processing: pre-emphasis
		sig_preemphasis( (short*)pEnrollBuf + start, (short*)pEnrollBuf + start, validSampleNum );

		// pCleanBuf = (char*)calloc( ( validSampleNum + start ) * BYTES_PER_SAMPLE, 1 );
		// denoise here
		// LOGE( "================denoise enroll ========start=======" );
		// cleanLen = denoise_mmse( (short*)pEnrollBuf, (short*)pCleanBuf , validSampleNum + start );
		// LOGE( "================denoise enroll ========stop=======" );

		LOGD( "!!!enrollment[%d] duration: %d samples, time slices: %d\n", enrollNum, validSampleNum, j );

		void *hFeatHandle = NULL;

		// init mfcc handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc MFCC feature
		error = mfcc16s32s_calc( hFeatHandle, (AUD_Int16s*)pEnrollBuf + start, validSampleNum, &(pEnrollFeat[enrollNum]) );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		sig_vaddeinit( &hVadHandle );

		char keywordWav[512] = { 0, };
		sprintf( keywordWav, "/sdcard/wov/keyword/keyword%d.wav", enrollNum );

		ret = writeWavToFile( (AUD_Int8s*)keywordWav, (AUD_Int16s*)pEnrollBuf + start, validSampleNum );
		AUD_ASSERT( ret == 0 );

		// free( pCleanBuf );
		// pCleanBuf = NULL;

		enrollNum++;

		if ( enrollNum < maxEnrollNum )
		{
			client->notifyCallback( ENROLL_SUCCESS, 1, maxEnrollNum - enrollNum );
		}
		else if ( enrollNum == maxEnrollNum )
		{
			if ( maxEnrollNum > 1 )
			{
				fnFuseTemplate();
			}
			else
			{
				ret = writeMatrixToFile( &(pEnrollFeat[0].featureMatrix), (AUD_Int8s*)keywordFile );
				AUD_ASSERT( ret == 0 );
			}

			for ( j = 0; j < maxEnrollNum; j++ )
			{
				ret = destroyMatrix( &(pEnrollFeat[j].featureMatrix) );
				AUD_ASSERT( ret == 0 );

				ret = destroyVector( &(pEnrollFeat[j].featureNorm) );
				AUD_ASSERT( ret == 0 );
			}


			free( pEnrollBuf );
			pEnrollBuf = NULL;

			enrollNum  = 0;

			client->notifyCallback( ENROLL_SUCCESS, 0, 0 );
		}

		LOGD( "Finish enroll!!!\n" );

		return 0;
	}

	WakeonVoice::WakeonVoice()
	{
		// get an instance for alsa recorder
		alsaRecorder = CAlsaRecorder::fnGetInstance();
		alsaRecorder->fnSetParameters( SAMPLE_RATE, CHANNEL_NUM, BYTES_PER_SAMPLE, VAD_FRAME_LEN );

		// apply for frame buffer
		frameBufLen = FRAME_LEN * BYTES_PER_SAMPLE;
		pFrameBuf = (char*)calloc( frameBufLen, 1 );
		AUD_ASSERT( pFrameBuf );

		listenRefCnt = 0;

		return;
	}

	WakeonVoice::~WakeonVoice()
	{
		free( pFrameBuf );
		pFrameBuf = NULL;

		alsaRecorder->fnStop();

		if ( pEnrollFeat )
		{
			free( pEnrollFeat );
			pEnrollFeat = NULL;
		}

		return;
	}

	status_t WakeonVoice::fnStartEnroll()
	{
		Mutex::Autolock _l( mLock );

		int ret = 0;

		enrollBufLen = MAX_ENROLL_SAMPLE_NUM * BYTES_PER_SAMPLE;
		pEnrollBuf   = (char*)calloc( enrollBufLen, 1 );
		AUD_ASSERT( pEnrollBuf );

		// init VAD handle
		ret = sig_vadinit( &hVadHandle, (AUD_Int16s*)pEnrollBuf, NULL, enrollBufLen, 6, 0 );
		AUD_ASSERT( ret == 0 );

		fnWorker = fnExtractTemplate;
		alsaRecorder->fnRegisterCallback( fnFrameAdmissionCallback );
		ret = alsaRecorder->fnStart();
		if ( ret != OK )
		{
			LOGE( "start alsa recorder failed\n" );
			return -1;
		}

		return OK;
	}

	int WakeonVoice::fnEnableDebug( int i )
	{
		enableDebug = i;

		LOGD( "debug flag set to %d\n", enableDebug );

		return 0;
	}

	int WakeonVoice::fnSetEnrollTimes( int t )
	{
		AUD_ASSERT( t > 0 );

		maxEnrollNum = t;
		if ( pEnrollFeat )
		{
			free( pEnrollFeat );
			pEnrollFeat = NULL;
		}
		pEnrollFeat = (AUD_Feature*)calloc( maxEnrollNum * sizeof(AUD_Feature), 1 );
		AUD_ASSERT( pEnrollFeat );

		LOGD( "########## set enroll number as: %d ##############", maxEnrollNum );

		return 0;
	}

	int WakeonVoice::fnSetSecurityLevel( int s )
	{
		LOGD( "================ user has choosen the security level to %d ================", s );
		if ( s == 0 )      // level: strict
		{
			threshold = 0.20;
		}
		else if ( s == 1 ) // level: medium, default
		{
			threshold = 0.29;
		}
		else if ( s == 2 ) // level: weak
		{
			threshold = 0.35;
		}
		LOGD( "=========threshold changed to %.2f ==========", threshold );
		return 0;
	}

	status_t WakeonVoice::fnStartListen()
	{
		Mutex::Autolock _l( mLock );

		if ( listenRefCnt > 0 )
		{
			LOGE( "recognition started -- bypass mode" );
			listenRefCnt++;
			return 0;
		}

		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

		// load template features
		ret = readMatrixFromFile( &(keywordFeat.featureMatrix), (AUD_Int8s*)keywordFile );
		AUD_ASSERT( ret == 0 );

		// min permitted utterance length
		// AUDLOG( "keyword frame feature rows: %d\n", keywordFeat.featureMatrix.rows );
		minLen = (int)round( keywordFeat.featureMatrix.rows * 0.5 );

		keywordFeat.featureNorm.len      = keywordFeat.featureMatrix.rows;
		keywordFeat.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(keywordFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		for ( int j = 0; j < keywordFeat.featureNorm.len; j++ )
		{
			keywordFeat.featureNorm.pInt64s[j] = 0;
			double tmp = 0.;
			for ( int k = 0; k < keywordFeat.featureMatrix.cols; k++ )
			{
				tmp += (double)( *( keywordFeat.featureMatrix.pInt32s + j * keywordFeat.featureMatrix.cols + k ) ) *
				       (double)( *( keywordFeat.featureMatrix.pInt32s + j * keywordFeat.featureMatrix.cols + k ) );
			}

			keywordFeat.featureNorm.pInt64s[j] = (long long)sqrt( tmp );
		}

		// maxmum allowrance 2x data
		streamBufLen = (AUD_Int32s)round( keywordFeat.featureNorm.len * FRAME_STRIDE * BYTES_PER_SAMPLE * 2.6 );
		pStreamBuf = (char*)calloc( streamBufLen, 1 );
		AUD_ASSERT( pStreamBuf );

		// init VAD handle
		ret = sig_vadinit( &hVadHandle, (AUD_Int16s*)pStreamBuf, NULL, streamBufLen, -1, 10 );
		AUD_ASSERT( ret == 0 );

		fnWorker = fnKeywordDetection;
		alsaRecorder->fnRegisterCallback( fnFrameAdmissionCallback );
		ret = alsaRecorder->fnStart();
		if ( ret != OK )
		{
			LOGE( "start alsa recorder failed\n" );
			return ret;
		}

		listenRefCnt++;

		LOGE( "recognition started" );

		return ret;
	}

	status_t WakeonVoice::fnStopListen()
	{
		Mutex::Autolock _l( mLock );

		if ( listenRefCnt == 1 )
		{
			int ret;

			alsaRecorder->fnStop();
			alsaRecorder->fnRegisterCallback( NULL );

			ret = destroyMatrix( &(keywordFeat.featureMatrix) );
			AUD_ASSERT( ret == 0 );

			ret = destroyVector( &(keywordFeat.featureNorm) );
			AUD_ASSERT( ret == 0 );

			sig_vaddeinit( &hVadHandle );

			free( pStreamBuf );
			pStreamBuf = NULL;
		}

		if ( listenRefCnt > 0 )
		{
			listenRefCnt--;
		}

		LOGE( "recognition stopped" );

		return OK;
	}

	status_t WakeonVoice::fnRegisterClient( sp<IWakeonVoiceClient> &c )
	{
		LOGD( "register client" );
		c->asBinder()->linkToDeath( this );
		client = c;

		return OK;
	}

	void WakeonVoice::binderDied( const wp<IBinder>& who )
	{
		LOGE( "!!!WakeonVoice Client died\n" );

		return;
	}

}

