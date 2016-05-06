/***************************************************************
* Intel MCG PSI Core TR project: mAudio, where m means magic
*   Author: Yao, Matrix( matrix.yao@intel.com )
* Copyright(c) 2012 Intel Corporation
* ALL RIGHTS RESERVED
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <cutils/properties.h>

#include <sys/types.h>
#include <dirent.h>

#include "ISpeakerVerification.h"
#include "SpeakerVerification.h"
#include "SpeakerVerificationClient.h"

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"
#include "math/mmath.h"
#include "io/streaming/android/AlsaRecorder.h"

#undef LOG_TAG
#define LOG_TAG "SpeakerVerification_NativeServer"
#include <utils/Log.h>

using namespace android;

#define PROP_THRESH "service.sv.threshold"
#define PROP_LOG    "service.sv.log"
#define SV_THRSHOLD "1.5"

#define PERF_PROFILE 0

#define TNORM_IMPOSTER_NUM 10
#define TOP_N              5


#define DUMP_INSTREAM  0
#if DUMP_INSTREAM
	FILE *fEnrollStream = NULL;
	FILE *fVerifyStream = NULL;
#endif

#define SV_VERIFICATION_FRAMENUM  3750
#define SV_ENROLL_MAX_FRAMENUM    7500

namespace speakerverification
{
	static sp<ISpeakerVerificationClient> client;
	static CAlsaRecorder                  *alsaRecorder;

	static char                           *pFrameBuf;
	static int                            frameBufLen;

	static double                         threshold;

	// models
	static void                           *hSpeaker                          = NULL;
	static double                         speakerScore                       = 0.;
	static void                           *hUbm                              = NULL;
	static double                         ubmScore                           = 0.;
	static void                           *hImposters[TNORM_IMPOSTER_NUM]    = { NULL, };
	static double                         imposterScores[TNORM_IMPOSTER_NUM] = { 0., };

	// frame admission handle
	static void                           *hVadHandle  = NULL;

	// feature extraction handle
	static void                           *hFeatHandle = NULL;
	static AUD_Feature                    frameFeat;
	static int                            frameFeatNum;
	static AUD_Matrix                     enrollFeatMatrix;

	static char                           userName[256];

	AUD_Tick                              t;

	static status_t (*fnWorker)( short*, int );

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

			fnWorker( (short*)pFrameBuf, FRAME_LEN );
		}

		return;
	}
	
	status_t fnSVProcess( short *pFrameBuf, int sampleLen )
	{
		// Mutex::Autolock _l( mLock );
		AUD_ASSERT( sampleLen == FRAME_LEN );

		int       i = 0, j = 0;
		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

#if DUMP_INSTREAM
		int data;
		data = fwrite( pFrameBuf, 1, FRAME_STRIDE * BYTES_PER_SAMPLE, fVerifyStream );
		fflush( fVerifyStream );
		// LOGD( "Wrote %d bytes\n", data );
#endif

		// frame admission
		AUD_Int16s flag;
		ret = sig_framecheck( hVadHandle, pFrameBuf, &flag );
		if ( ret != 1 )
		{
			return 0;
		}

		// Pre-processing: pre-emphasis
		sig_preemphasis( pFrameBuf, pFrameBuf, sampleLen );

		// TODO: frame based de-noise here

		// calc MFCC feature
		error = mfcc16s32s_calc( hFeatHandle, pFrameBuf, sampleLen, &frameFeat );
		if ( error == AUD_ERROR_MOREDATA )
		{
			return 0;
		}
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc gmm scores
		AUD_Vector compLlr;
		compLlr.len      = gmm_getmixnum( hUbm );
		compLlr.dataType = AUD_DATATYPE_DOUBLE;
		ret = createVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		AUD_Vector bestIndex;
		bestIndex.len      = TOP_N;
		bestIndex.dataType = AUD_DATATYPE_INT32S;
		ret = createVector( &bestIndex );
		AUD_ASSERT( ret == 0 );

		double frameScore;

		// calc ubm score
		frameScore = gmm_llr( hUbm, &(frameFeat.featureMatrix), 0, &compLlr );

		// get top N gaussian component
		// showVector( &compLlr );
		ret = sortVector( &compLlr, &bestIndex );
		AUD_ASSERT( ret == 0 );
		// showVector( &bestIndex );

		frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
		for( i = 1; i < bestIndex.len; i++ )
		{
			frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[i]] );
		}
		ubmScore += frameScore;

		// calc speaker score
		frameScore = gmm_llr( hSpeaker, &(frameFeat.featureMatrix), 0, &compLlr );

		frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
		for( i = 1; i < bestIndex.len; i++ )
		{
			frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[i]] );
		}
		speakerScore += frameScore;

		// calc imposter scores
		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			frameScore = gmm_llr( hImposters[i], &(frameFeat.featureMatrix), 0, &compLlr );

			frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
			for( j = 1; j < bestIndex.len; j++ )
			{
				frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
			}
			imposterScores[i] += frameScore;
		}

		frameFeatNum++;

		if ( frameFeatNum == SV_VERIFICATION_FRAMENUM )
		{
			double score;
			score = ( speakerScore - ubmScore ) / frameFeatNum;
			for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
			{
				imposterScores[i] = ( imposterScores[i] - ubmScore ) / frameFeatNum;
			}

			// T-Normalization
			AUD_Double mean = 0., std = 0.;
			for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
			{
				mean += imposterScores[i];
			}
			mean /= TNORM_IMPOSTER_NUM;

			for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
			{
				std += ( imposterScores[i] - mean ) * ( imposterScores[i] - mean );
			}
			std /= TNORM_IMPOSTER_NUM;
			std = sqrt( std );

			score = (score - mean) / std;

			char value[PROPERTY_VALUE_MAX];
			property_get( PROP_THRESH, value, SV_THRSHOLD );
			// LOGD( "threshold: %s", value );
			if ( threshold != atof( value ) )
			{
				threshold = atof( value );
				LOGD( "Threshold changed to: %.2f", threshold );
			}

			LOGD( "#####Utterance with score: %.2f\n", score );
			if ( score >= threshold )
			{
				client->notifyCallback( VERIFICATION_SUCCESS, 0, (int)(score * (1 << 16)) );
				LOGD( " -------------------------------------------------------Verification score is : %d",  (int)(score * (1 << 16)) );
			}
			else
			{
				client->notifyCallback( VERIFICATION_FAILURE, 0, (int)(score * (1 << 16)) );
				LOGD( " -------------------------------------------------------Verification score is : %d",  (int)(score * (1 << 16)) );
			}

			frameFeatNum = 0;

			// XXX: temporal smooth can add here if needed
		}

		ret = destroyVector( &bestIndex );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		return 0;
	}

	status_t fnEnrollProcess( short *pFrameBuf, int sampleLen )
	{
		// Mutex::Autolock _l( mLock );

		AUD_ASSERT( sampleLen == FRAME_LEN );

		int       j = 0;
		int       ret;
		AUD_Error error = AUD_ERROR_NONE;

#if DUMP_INSTREAM
		int data;
		data = fwrite( pFrameBuf, 1, FRAME_STRIDE * BYTES_PER_SAMPLE, fEnrollStream );
		fflush( fEnrollStream );
		// LOGD( "Wrote %d bytes\n", data );
#endif

		if ( frameFeatNum >= SV_ENROLL_MAX_FRAMENUM )
		{
			LOGD( "Dismiss this frame for sufficient enrollment" );
			return 0;
		}

		// frame admission
		AUD_Int16s flag;
		ret = sig_framecheck( hVadHandle, pFrameBuf, &flag );
		if ( ret != 1 )
		{
			return 0;
		}

		// Front end processing
		// Pre-processing: pre-emphasis
		sig_preemphasis( pFrameBuf, pFrameBuf, sampleLen );

		frameFeat.featureMatrix.pInt32s = enrollFeatMatrix.pInt32s + frameFeatNum * enrollFeatMatrix.cols;

		// calc mfcc feature
		error = mfcc16s32s_calc( hFeatHandle, pFrameBuf, sampleLen, &frameFeat );
		if ( error == AUD_ERROR_MOREDATA )
		{
			return 0;
		}
		AUD_ASSERT( error == AUD_ERROR_NONE );

		frameFeatNum++;

		// time out, stop enroll
		if ( frameFeatNum == SV_ENROLL_MAX_FRAMENUM )
		{
			// self stop
			client->notifyCallback( ENROLL_SUFFICIENT, 0, 0 );
		}

		return 0;
	}

	SpeakerVerification::SpeakerVerification()
	{
		// get an instance for alsa recorder
		alsaRecorder = CAlsaRecorder::fnGetInstance();

		alsaRecorder->fnSetParameters( SAMPLE_RATE, CHANNEL_NUM, BYTES_PER_SAMPLE, FRAME_STRIDE );

		// apply for frame buffer
		frameBufLen = FRAME_LEN * BYTES_PER_SAMPLE;
		pFrameBuf = (char*)calloc( frameBufLen, 1 );
		AUD_ASSERT( pFrameBuf );

		return;
	}

	SpeakerVerification::~SpeakerVerification()
	{
		free( pFrameBuf );
		pFrameBuf = NULL;

		alsaRecorder->fnStop();

#if DUMP_INSTREAM
		if ( fVerifyStream )
		{
			fclose( fVerifyStream );
			fVerifyStream = NULL;
		}
		if ( fEnrollStream )
		{
			fclose( fEnrollStream );
			fEnrollStream = NULL;
		}
#endif

		return;
	}

	status_t SpeakerVerification::fnStartEnroll( char *user )
	{
		Mutex::Autolock _l( mLock );

		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

		// load ubm model
		if ( hUbm == NULL )
		{
			FILE *fpUbm = fopen( SV_UBM_GMMMODEL_FILE, "rb" );
			if ( fpUbm == NULL )
			{
				AUDLOG( "Cannot open ubm model file: [%s]\n", SV_UBM_GMMMODEL_FILE );
				return AUD_ERROR_IOFAILED;
			}
			error = gmm_import( &hUbm, fpUbm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			fclose( fpUbm );
			fpUbm = NULL;
		}

		// init feature extraction handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_LEN, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// init frame feature
		frameFeat.featureMatrix.rows     = 1;
		frameFeat.featureMatrix.cols     = MFCC_FEATDIM;
		frameFeat.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		frameFeat.featureMatrix.pInt32s  = NULL;

		frameFeat.featureNorm.len      = 1;
		frameFeat.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(frameFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// init enroll feature matrix
		enrollFeatMatrix.rows          = SV_ENROLL_MAX_FRAMENUM;
		enrollFeatMatrix.cols          = MFCC_FEATDIM;
		enrollFeatMatrix.dataType      = AUD_DATATYPE_INT32S;
		ret = createMatrix( &enrollFeatMatrix );
		AUD_ASSERT( ret == 0 );

		// init frame admission handle
		ret = sig_vadinit( &hVadHandle, NULL, NULL, 0, -1, 6 );
		AUD_ASSERT( ret == 0 );

		snprintf( userName, 255, "%s", user );
		LOGE( "##### new user name: %s #####", user );

		frameFeatNum = 0;

#if DUMP_INSTREAM
		char dumpName[256] = { 0, };
		snprintf( dumpName, 255, "/sdcard/sv/%s.pcm", userName );
		fEnrollStream = fopen( dumpName, "wb" );
		AUD_ASSERT( fEnrollStream );
#endif

		fnWorker = fnEnrollProcess;
		alsaRecorder->fnRegisterCallback( fnFrameAdmissionCallback );
		ret = alsaRecorder->fnStart();
		if ( ret != OK )
		{
			LOGE( "Start alsa recorder failed\n" );
		}

		return OK;
	}

	status_t SpeakerVerification::fnGenerateModel()
	{
		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

		// map speaker model
		void      *hAdaptedGmm        = NULL;
		AUD_Int8s adaptedGmmName[256] = { 0, };
		snprintf( (char*)adaptedGmmName, 256, "%s", userName );
		error = gmm_clone( &hAdaptedGmm, hUbm, adaptedGmmName );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		AUD_Matrix trainMatrix;
		trainMatrix.rows     = frameFeatNum;
		trainMatrix.cols     = MFCC_FEATDIM;
		trainMatrix.dataType = AUD_DATATYPE_INT32S;
		trainMatrix.pInt32s  = enrollFeatMatrix.pInt32s;

		// adapt GMM
		error = gmm_adapt( hAdaptedGmm, &trainMatrix );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// gmm_show( hAdaptedGmm );

		// export gmm
		char modelFile[256] = { 0 };
		snprintf( modelFile, 256, "%s/%s.gmm", SV_SPEAKER_GMMMODEL_DIR, userName );
		LOGE( "Export GMM Model to: %s\n", modelFile );
		FILE *fpGmm = fopen( modelFile, "wb" );
		AUD_ASSERT( fpGmm );

		error = gmm_export( hAdaptedGmm, fpGmm );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		LOGE( "Export GMM Model File Done\n" );

		fclose( fpGmm );
		fpGmm = NULL;

		error = gmm_free( &hUbm );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &enrollFeatMatrix );
		AUD_ASSERT( ret == 0 );

		client->notifyCallback( ENROLL_SUCCESS, 1, 1 );

		LOGE( "Speaker Model Generate Done" );

		return OK;
	}

	status_t SpeakerVerification::fnStopEnroll()
	{
		Mutex::Autolock _l( mLock );

		int       ret   = 0;
		AUD_Error error = AUD_ERROR_NONE;

		alsaRecorder->fnStop();
		alsaRecorder->fnRegisterCallback( NULL );
		fnWorker = NULL;

		sig_vaddeinit( &hVadHandle );

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// XXX: clean frame feature
		ret = destroyVector( &(frameFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		sp<GenerateModelThread> thread = new GenerateModelThread( this );

#if DUMP_INSTREAM
		fclose( fEnrollStream );
		fEnrollStream = NULL;
#endif

		LOGE( "Enrollment stopped" );

		return OK;
	}

	status_t SpeakerVerification::fnStartListen( char *user )
	{
		Mutex::Autolock _l( mLock );

		int       ret   = 0, i = 0;
		AUD_Error error = AUD_ERROR_NONE;

		snprintf( userName, 255, "%s", user );
		LOGE( "##### declared user name: %s #####", user );

		speakerScore = 0.;
		ubmScore     = 0.;
		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			imposterScores[i] = 0.;
		}
		frameFeatNum = 0;

		// load speaker model
		char modelName[256] = { 0, };
		snprintf( modelName, 256, "%s.gmm", userName );
		char modelFile[256] = { 0 };
		snprintf( modelFile, 256, "%s/%s", SV_SPEAKER_GMMMODEL_DIR, modelName );
		FILE *fpSpeaker = fopen( modelFile, "rb" );
		if ( fpSpeaker == NULL )
		{
			AUDLOG( "Cannot open speaker model file: [%s]\n", modelFile );
			return AUD_ERROR_IOFAILED;
		}
		error = gmm_import( &hSpeaker, fpSpeaker );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		fclose( fpSpeaker );
		fpSpeaker = NULL;
		AUDLOG( "import speaker model from: [%s]\n", modelFile );

		// load ubm model
		if ( hUbm == NULL )
		{
			FILE *fpUbm = fopen( SV_UBM_GMMMODEL_FILE, "rb" );
			if ( fpUbm == NULL )
			{
				AUDLOG( "Cannot open ubm model file: [%s]\n", SV_UBM_GMMMODEL_FILE );
				return AUD_ERROR_IOFAILED;
			}
			error = gmm_import( &hUbm, fpUbm );
			AUD_ASSERT( error == AUD_ERROR_NONE );
			fclose( fpUbm );
			fpUbm = NULL;
		}

		// load imposter models
		DIR           *pImposterDir   = opendir( (const char*)SV_SPEAKER_GMMMODEL_DIR );
		struct dirent *pImposterEntry = NULL;
		AUD_Int32s    imposterNum     = 0;
		while ( ( pImposterEntry = readdir( pImposterDir ) ) && imposterNum < TNORM_IMPOSTER_NUM )
		{
			if ( strstr( pImposterEntry->d_name, ".gmm" ) == NULL || !strcmp( pImposterEntry->d_name, modelName ) )
			{
				continue;
			}

			char imposterFile[256] = { 0, };
			snprintf( imposterFile, 256, "%s/%s", (const char*)SV_SPEAKER_GMMMODEL_DIR, pImposterEntry->d_name );

			FILE *fpImposter  = fopen( imposterFile, "rb" );
			if ( fpImposter == NULL )
			{
				AUDLOG( "cannot open imposter model file: [%s]\n", imposterFile );
				return AUD_ERROR_IOFAILED;
			}

			AUDLOG( "import imposter[%d] model from: [%s]\n", imposterNum, imposterFile );
			error = gmm_import( &(hImposters[imposterNum]), fpImposter );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			fclose( fpImposter );
			fpImposter = NULL;

			imposterNum++;
		}
		closedir( pImposterDir );
		pImposterDir   = NULL;
		pImposterEntry = NULL;
		AUD_ASSERT( imposterNum == TNORM_IMPOSTER_NUM );

		// init frame feature
		frameFeat.featureMatrix.rows          = 1;
		frameFeat.featureMatrix.cols          = MFCC_FEATDIM;
		frameFeat.featureMatrix.dataType      = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(frameFeat.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		frameFeat.featureNorm.len      = 1;
		frameFeat.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(frameFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// init mfcc handle
		error = mfcc16s32s_init( &hFeatHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_LEN, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// init frame admission handle
		ret = sig_vadinit( &hVadHandle, NULL, NULL, 0, -1, 6 );
		AUD_ASSERT( ret == 0 );

#if DUMP_INSTREAM
		char dumpName[256] = { 0, };
		snprintf( dumpName, 255, "/sdcard/sv/declare_%s.pcm", userName );
		fVerifyStream = fopen( dumpName, "wb" );
		AUD_ASSERT( fVerifyStream );
#endif

		fnWorker = fnSVProcess;
		alsaRecorder->fnRegisterCallback( fnFrameAdmissionCallback );
		ret = alsaRecorder->fnStart();
		if ( ret != OK )
		{
			LOGE( "Start alsa recorder failed( %s, %s, %d )\n", __FILE__, __FUNCTION__, __LINE__ );
		}

		return ret;
	}

	status_t SpeakerVerification::fnStopListen()
	{
		Mutex::Autolock _l( mLock );

		AUD_Error error = AUD_ERROR_NONE;
		int       i, ret;
		double    mean = 0., std = 0.;
		double    score;
		char      value[PROPERTY_VALUE_MAX];

		alsaRecorder->fnStop();
		alsaRecorder->fnRegisterCallback( NULL );
		fnWorker = NULL;

		sig_vaddeinit( &hVadHandle );

		if ( frameFeatNum <= 0 )
		{
			client->notifyCallback( VERIFICATION_FAILURE, 1, -1000 * 65536 );
			goto exit_stoplisten;
		}

		score = ( speakerScore - ubmScore ) / frameFeatNum;
		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			imposterScores[i] = ( imposterScores[i] - ubmScore ) / frameFeatNum;
		}

		// T-Normalization
		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			mean += imposterScores[i];
		}
		mean /= TNORM_IMPOSTER_NUM;

		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			std += ( imposterScores[i] - mean ) * ( imposterScores[i] - mean );
		}
		std /= TNORM_IMPOSTER_NUM;
		std = sqrt( std );

		score = (score - mean) / std;

		property_get( PROP_THRESH, value, SV_THRSHOLD );
		// LOGD( "threshold: %s", value );
		if ( threshold != atof( value ) )
		{
			threshold = atof( value );
			LOGD( "Threshold changed to: %.2f", threshold );
		}

		LOGD( "#####Utterance with score: %.2f, with frame number: %d\n", score, frameFeatNum );
		if ( score >= threshold )
		{
			LOGD( " -------------------------------------------------------Verification score is : %d",  (int)(score * (1 << 16)) );
			client->notifyCallback( VERIFICATION_SUCCESS, 0, (int)(score * (1 << 16)) );
		}
		else
		{
			LOGD( " -------------------------------------------------------Verification score is : %d",  (int)(score * (1 << 16)) );
			client->notifyCallback( VERIFICATION_FAILURE, 0, (int)(score * (1 << 16)) );
		}

exit_stoplisten:
		frameFeatNum = 0;

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hFeatHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = gmm_free( &hUbm );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = gmm_free( &hSpeaker );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		for ( i = 0; i < TNORM_IMPOSTER_NUM; i++ )
		{
			error = gmm_free( &(hImposters[i]) );
			AUD_ASSERT( error == AUD_ERROR_NONE );
		}

		// destroy feature
		ret = destroyMatrix( &(frameFeat.featureMatrix) );
		AUD_ASSERT( ret == 0 );
		ret = destroyVector( &(frameFeat.featureNorm) );
		AUD_ASSERT( ret == 0 );

#if DUMP_INSTREAM
		fclose( fVerifyStream );
		fVerifyStream = NULL;
#endif

		LOGE( "Stop listen done" );

		return OK;
	}

	status_t SpeakerVerification::fnRegisterClient( sp<ISpeakerVerificationClient> &c )
	{
		LOGD( "register client" );
		c->asBinder()->linkToDeath( this );
		client = c;

		return OK;
	}

	void SpeakerVerification::binderDied( const wp<IBinder>& who )
	{
		LOGE( "!!!SpeakerVerification Client died\n" );

		return;
	}

}

