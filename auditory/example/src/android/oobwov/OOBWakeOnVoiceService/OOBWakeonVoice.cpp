#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <cutils/properties.h>

#include "IOOBWakeonVoice.h"
#include "OOBWakeonVoice.h"
#include "OOBWakeonVoiceClient.h"

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
#define LOG_TAG "OOBWakeOnVoice_NativeServer"
#include <utils/Log.h>

#define PROP_THRESH "service.oobwov.threshold"
#define PROP_LOG    "service.oobwov.log"

using namespace android;

#define PERF_PROFILE      0



#define MAX_ENROLL_SAMPLE_NUM  (SAMPLE_RATE * 4)

namespace oobwakeonvoice
{
	static sp<IOOBWakeonVoiceClient> client;
	static CAlsaRecorder             *alsaRecorder;

	static char                   *pEnrollBuf;
	static int                    enrollBufLen;
	static char                   *pStreamBuf;
	static int                    streamBufLen;
	static char                   *pFrameBuf;
	static int                    frameBufLen;
	static char                   *pCleanBuf;
	static int                    cleanLen;
	static double                 threshold;
	static void                   *hVadHandle      = NULL;
	static void                   *hImposter       = NULL;
	static void                   *hSpeaker        = NULL;
	static AUD_Matrix             totalFeatureMatrix;
	static int                    minLen           = 40;
	static int                    listenRefCnt     = 0;
	static int                    enrollNum        = 0;
	static int                    maxEnrollNum     = 1;
	static int                    enableDebug      = 0;
	static int                    currentRow       = 0;
	static int                    oobMode          = 1;

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
//=============================================================================================================//
			validSampleNum = sig_vadupdate( hVadHandle, (short*)pFrameBuf, &start );

			// because it need smooth strategy to enroll two words together, tail of end is not real speech. Give up 0.16s.
			// validSampleNum = validSampleNum - 0.16 * SAMPLE_RATE;

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

		void            *hFeatHandle = NULL;
		// init mfcc handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc MFCC feature
		// error = mfcc16s32s_calc( hFeatHandle, (short*)pCleanBuf, cleanLen, &inFeature );

		error = mfcc16s32s_calc( hFeatHandle, (short*)(pStreamBuf + start), validSampleNum, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		AUD_Double imposterScore = 0., speakerScore = 0.;
		AUD_Double frameScore    = 0., finalScore   = 0.;

		AUD_Vector compLlr;
		compLlr.len      = gmm_getmixnum( hImposter );
		compLlr.dataType = AUD_DATATYPE_DOUBLE;
		ret = createVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		int i = 0;
		for ( i = 0; i < inFeature.featureMatrix.rows; i++ )
		{
			frameScore = gmm_llr( hImposter, &(inFeature.featureMatrix), i, &compLlr );
			imposterScore += frameScore;
			frameScore = gmm_llr( hSpeaker, &(inFeature.featureMatrix), i, &compLlr );
			speakerScore += frameScore;
		}
		imposterScore /= inFeature.featureMatrix.rows;
		speakerScore  /= inFeature.featureMatrix.rows;
		finalScore = speakerScore - imposterScore;

		AUDLOG( "\tkeyword score: %.2f, imposter score: %.2f, final score: %.2f\n", speakerScore, imposterScore, finalScore );

		LOGD( "=================#####utterance with score: %.2f\n", finalScore );

		if ( enableDebug == 1 )
		{
			char fileName[512] = { 0, };
			unsigned int  t    = (unsigned int)( systemTime() / 1000000 );
			sprintf( fileName, "/sdcard/wov/debug/oob/%u_%d_%.2f.wav", t, oobMode, finalScore );
			ret = writeWavToFile( (AUD_Int8s*)fileName, (short*)pStreamBuf, validSampleNum + start );
			AUD_ASSERT( ret == 0 );
			LOGD( "==================debug file write to %s===================\n", fileName );
		}


		// int t = systemTime();
		// LOGE( "#####BEFORE CALLBACK\n" );

		// char value[PROPERTY_VALUE_MAX];
		/*if ( oobMode == 1 )
		{
			property_get( PROP_THRESH, value, OOBWOV_THRESHOLD );
			// LOGD( "threshold: %s", value );
			if ( threshold != atof( value ) )
			{
				threshold = atof( value );
				LOGD( "threshold changed to: %.2f", threshold );
			}
		}
		else
		{
			property_get( PROP_THRESH, value, SPEAKERWOV_THRESHOLD );
			if ( threshold != atof( value ) )
			{
				threshold = atof( value );
				LOGD( "threshold changed to: %.2f", threshold );
			}
		}
		*/

		if ( finalScore > threshold )
		{	
			client->notifyCallback( WAKE_ON_KEYWORD, oobMode, 1 );
		}

		// t = systemTime() - t;
		// LOGE( "#####notify callback time: %.2f ms\n", t / 1000000.0 );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &compLlr );
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

		minLen += j;

		// temp feature matrix
		AUD_Feature feature;
		feature.featureMatrix.rows     = j - MFCC_DELAY;
		feature.featureMatrix.cols     = MFCC_FEATDIM;
		feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		feature.featureMatrix.pInt32s  = totalFeatureMatrix.pInt32s + currentRow * feature.featureMatrix.cols;

		feature.featureNorm.len        = feature.featureMatrix.rows;
		feature.featureNorm.dataType   = AUD_DATATYPE_INT64S;
		ret = createVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// Pre-processing: pre-emphasis
		sig_preemphasis( (short*)pEnrollBuf + start, (short*)pEnrollBuf + start, validSampleNum );

		LOGD( "!!!enrollment[%d] duration: %d samples, time slices: %d\n", enrollNum, validSampleNum, j );

		void *hFeatHandle = NULL;

		// init mfcc handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc MFCC feature
		error = mfcc16s32s_calc( hFeatHandle, (AUD_Int16s*)pEnrollBuf + start, validSampleNum, &feature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit vad handle
		sig_vaddeinit( &hVadHandle );

		char keywordWav[512] = { 0, };
		sprintf( keywordWav, "/sdcard/wov/keyword/oob/keyword%d.wav", enrollNum );
		ret = writeWavToFile( (AUD_Int8s*)keywordWav, (AUD_Int16s*)pEnrollBuf + start, validSampleNum );
		AUD_ASSERT( ret == 0 );

		enrollNum++;

		currentRow += feature.featureMatrix.rows;
		AUD_ASSERT( currentRow < totalFeatureMatrix.rows );

		ret = destroyVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		if ( enrollNum < maxEnrollNum )
		{
			client->notifyCallback( ENROLL_SUCCESS, 1, maxEnrollNum - enrollNum );
		}
		else if ( enrollNum == maxEnrollNum )
		{
			FILE *fpKeyword = fopen( WOV_KEYWORD_GMMMODEL_FILE, "rb" );
			if ( fpKeyword == NULL )
			{
				AUDLOG( "cannot open ubm model file" );
				return AUD_ERROR_IOFAILED;
			}
			void *hAdaptedGmm     = NULL;
			error = gmm_import( &hAdaptedGmm, fpKeyword );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			fclose( fpKeyword );
			fpKeyword = NULL;

			AUD_Matrix usefulMatrix;
			usefulMatrix.rows     = currentRow;
			usefulMatrix.cols     = MFCC_FEATDIM;
			usefulMatrix.dataType = AUD_DATATYPE_INT32S;
			usefulMatrix.pInt32s  = totalFeatureMatrix.pInt32s;

			// adapt GMM
			error = gmm_adapt( hAdaptedGmm, &usefulMatrix );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			// export gmm
			FILE *fpGmm = fopen( WOV_SPEAKER_GMMMODEL_FILE, "wb" );
			AUD_ASSERT( fpGmm );

			error = gmm_export( hAdaptedGmm, fpGmm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			AUDLOG( "Export GMM Model File Done\n" );

			fclose( fpGmm );
			fpGmm = NULL;

			gmm_free( &hAdaptedGmm );

			free( pEnrollBuf );
			pEnrollBuf = NULL;

			ret = destroyMatrix( &totalFeatureMatrix );
			AUD_ASSERT( ret == 0 );

			minLen /= maxEnrollNum;
			minLen = (int)( minLen * 0.8 + 0.5 );

			enrollNum  = 0;
			currentRow = 0;

			client->notifyCallback( ENROLL_SUCCESS, 0, 0 );
		}

		LOGD( "Finish enroll!!!\n" );

		return 0;
	}

	OOBWakeonVoice::OOBWakeonVoice()
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

	OOBWakeonVoice::~OOBWakeonVoice()
	{
		free( pFrameBuf );
		pFrameBuf = NULL;
		alsaRecorder->fnStop();

		return;
	}

	status_t OOBWakeonVoice::fnStartEnroll()
	{
		Mutex::Autolock _l( mLock );

		int ret = 0;
		if ( enrollNum == 0 )
		{
			// create AUD_Matrix totalFeatureMatrix
			totalFeatureMatrix.rows     = 100 * maxEnrollNum; // "hello intel" would cost about 29~32 rows
			totalFeatureMatrix.cols     = MFCC_FEATDIM;
			totalFeatureMatrix.dataType = AUD_DATATYPE_INT32S;
			ret = createMatrix( &totalFeatureMatrix );
			AUD_ASSERT( ret == 0 );
	
			minLen = 0;
		}

		enrollBufLen = MAX_ENROLL_SAMPLE_NUM * BYTES_PER_SAMPLE;
		pEnrollBuf   = (char*)calloc( enrollBufLen, 1 );
		AUD_ASSERT( pEnrollBuf );

		// init VAD handle
		ret = sig_vadinit( &hVadHandle, (AUD_Int16s*)pEnrollBuf, NULL, enrollBufLen, 6, 6 );
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

	int OOBWakeonVoice::fnEnableDebug( int i )
	{
		enableDebug = i;

		LOGD( "debug flag set to %d\n", enableDebug );

		return 0;
	}

	int OOBWakeonVoice::fnSetEnrollTimes( int t )
	{
		AUD_ASSERT( t > 0 );

		maxEnrollNum = t;
		LOGD( "########## set enroll number as: %d ##############", maxEnrollNum );

		return 0;
	}

	int OOBWakeonVoice::fnSetSecurityLevel( int s )
	{
		LOGD( "============== user has set security level to %d ============", s );
		if ( oobMode == 1 )    // OOB MODE
		{
			if ( s == 0 )      // level: strict
			{
				threshold = 2.5;
			}
			else if ( s == 1 ) // level: medium, default
			{
				threshold = 1.5;
			}
			else if ( s == 2 ) // level: weak
			{
				threshold = 0.5;
			}
		}
		else                   // SPEAKER MODE
		{
			if ( s == 0 )      // level: strict
			{
				threshold = 5.0;
			}
			else if ( s == 1 ) // level: medium, default
			{
				threshold = 4.0;
			}
			else if ( s == 2 ) // level: weak
			{
				threshold = 3.0;
			}
		}

		LOGD( "=========threshold changed to %.2f ==========", threshold );
		return 0;
	}

	status_t OOBWakeonVoice::fnStartListen()
	{
		int ret = 0;
		Mutex::Autolock _l( mLock );
		AUD_Error error = AUD_ERROR_NONE;
		if ( listenRefCnt > 0 )
		{
			LOGE( "recognition started -- bypass mode" );
			listenRefCnt++;
			return 0;
		}
		FILE *fpImposter  = fopen( WOV_IMPOSTER_GMMMODEL_FILE, "rw" );
		if ( fpImposter == NULL )
		{
			AUDLOG( "cannot open imposter.gmm" );
			return AUD_ERROR_IOFAILED;
		}
		error = gmm_import( &hImposter, fpImposter );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpImposter );
		fpImposter = NULL;

		oobMode = 1;
		FILE *fpSpeaker = fopen( WOV_SPEAKER_GMMMODEL_FILE, "rw" );
		if ( fpSpeaker == NULL )
		{
			fpSpeaker = fopen( WOV_KEYWORD_GMMMODEL_FILE, "rw" );
			if ( fpSpeaker == NULL )
			{
				AUDLOG( "cannot open speaker & keyword model, pls check whether exists" );
				AUD_ASSERT( 0 );
				return AUD_ERROR_IOFAILED;
			}
		}
		else
		{
			oobMode = 0;
		}

		error = gmm_import( &hSpeaker, fpSpeaker );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpSpeaker );
		fpSpeaker = NULL;

		streamBufLen = 56000;
		pStreamBuf = (char*)calloc( streamBufLen, 1 );
		AUD_ASSERT( pStreamBuf );

		// init VAD handle
		ret = sig_vadinit( &hVadHandle, (AUD_Int16s*)pStreamBuf, NULL, streamBufLen, 6, 6 );
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

	status_t OOBWakeonVoice::fnStopListen()
	{
		Mutex::Autolock _l( mLock );

		if ( listenRefCnt == 1 )
		{
			int       ret;
			AUD_Error error;

			alsaRecorder->fnStop();
			alsaRecorder->fnRegisterCallback( NULL );

			error = gmm_free( &hSpeaker );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = gmm_free( &hImposter );
			AUD_ASSERT( error == AUD_ERROR_NONE );

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

	status_t OOBWakeonVoice::fnRegisterClient( sp<IOOBWakeonVoiceClient> &c )
	{
		LOGD( "register client" );
		c->asBinder()->linkToDeath( this );
		client = c;

		return OK;
	}

	void OOBWakeonVoice::binderDied( const wp<IBinder>& who )
	{
		LOGE( "!!!OOBWakeonVoice Client died\n" );

		return;
	}

}

