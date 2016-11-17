#include <utils/Log.h>
#undef LOG_TAG
#define LOG_TAG "AlsaRecorder"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <unistd.h>

#include "AlsaRecorder.h"

namespace android
{
// define the static parameter in alsarecord class
CAlsaRecorder*       CAlsaRecorder::s_pAlsaInstance    = NULL;

char*                CAlsaRecorder::s_pDataBuf         = NULL;
int                  CAlsaRecorder::s_nDataBufLen      = 0;
int                  CAlsaRecorder::s_nRecordByte      = 0;
int                  CAlsaRecorder::s_nReadPos         = 0;
int                  CAlsaRecorder::s_nWritePos        = 0;

char*                CAlsaRecorder::s_pPeriodBuf       = NULL;

snd_pcm_t*           CAlsaRecorder::s_pHandle          = NULL;
snd_pcm_hw_params_t* CAlsaRecorder::s_pParams          = NULL;


int                  CAlsaRecorder::s_nChannel         = 0;
int                  CAlsaRecorder::s_nSamplesPerFrame = 0;
int                  CAlsaRecorder::s_nBytesPerFrame   = 0;
int                  CAlsaRecorder::s_nSampleRate      = 0;
int                  CAlsaRecorder::s_nbytesPerSample  = 0;
int                  CAlsaRecorder::s_nStride          = 0;

void(*CAlsaRecorder::s_fnCallback)()                   = NULL;


#define DEBUG_ALSA_DUMP 0

#if DEBUG_ALSA_DUMP
FILE*                alsaDumpFp                        = NULL;
int                  alsaDumpNum                       = 0;
#endif

/*  @Description : get the PCM raw data from alsa driver and call back the registered function
    @Parameters  : NULL
    @Return      : void
    @Note        : N/A
*/
void* CAlsaRecorder::fnRecordThread( )
{
	int ret, i;

	if ( s_pHandle == NULL )
	{
		return NULL;
	}

	ret = snd_pcm_readi( s_pHandle, s_pPeriodBuf, s_nSamplesPerFrame );
	// LOGE( "ret: %d\n", ret );
	if ( ret == -EPIPE )
	{
		/* EPIPE means overrun */
		LOGE( "!!!!!!overrun occurred\n" );
		snd_pcm_prepare( s_pHandle );
	}
	else if ( ret <= 0 )
	{
		LOGE( "error from read: %s\n", snd_strerror( ret ) );
	}
	else if ( ret != s_nSamplesPerFrame )
	{
		// TODO: error handling
		LOGE( "short read, read %d frames\n", ret );
	}

	if ( ret > 0 )
	{
		Mutex::Autolock lock( mLock );

		if ( s_pDataBuf )
		{
			for ( i = 0; i < ret * s_nbytesPerSample; i++ )
			{
				s_pDataBuf[s_nWritePos] = s_pPeriodBuf[i];
				s_nWritePos = ( s_nWritePos + 1 ) % s_nDataBufLen;
			}
			s_nRecordByte += ret * s_nbytesPerSample;
		}

#if DEBUG_ALSA_DUMP
		fwrite( s_pPeriodBuf, ret * s_nbytesPerSample, 1, alsaDumpFp );
#endif
	}

	if ( s_nRecordByte >= s_nBytesPerFrame && s_fnCallback != NULL )
	{
		// LOGE( "before callback: s_nWritePos: %d, s_nReadPos: %d\n", s_nWritePos, s_nReadPos );
		s_fnCallback();
		// LOGE( "after callback: s_nWritePos: %d, s_nReadPos: %d\n", s_nWritePos, s_nReadPos );
	}

	return NULL;
}

CAlsaRecorder::CAlsaRecorder()
{
	// default values
	// sample rate : 16k
	s_nSampleRate      = 16000;
	// channel: mono
	s_nChannel         = 1;

	s_nbytesPerSample  = 2;
	s_nStride          = s_nSamplesPerFrame;

	// frame size
	s_nSamplesPerFrame = 512;

	// bytes per frame
	s_nBytesPerFrame   = s_nSamplesPerFrame * s_nbytesPerSample;

	s_nRecordByte = 0;
	s_nReadPos    = 0;
	s_nWritePos   = 0;
	s_nDataBufLen = 0;

	mRecordThread = NULL;
}

CAlsaRecorder::~CAlsaRecorder()
{
	s_nRecordByte = 0;

	if ( s_pDataBuf )
	{
		free( s_pDataBuf );
		s_pDataBuf = NULL;
	}
	if ( s_pPeriodBuf )
	{
		free( s_pPeriodBuf );
		s_pPeriodBuf = NULL;
	}
}


/*  @Description : singleton partern
    @Parameters  : NULL
    @Return      : CAlsaRecorder*
                   the object or reference of CAlsaRecorder
    @Note        : N/A
*/
CAlsaRecorder* CAlsaRecorder::fnGetInstance()
{
	if ( s_pAlsaInstance == NULL )
	{
		s_pAlsaInstance = new CAlsaRecorder();
	}

	return s_pAlsaInstance;
}

status_t CAlsaRecorder::fnRegisterCallback( void(*fnCb)() )
{
	Mutex::Autolock lock( mLock );

	s_fnCallback = fnCb;

	return OK;
}

status_t CAlsaRecorder::fnSetParameters( int sampleRate, int channelNum, int bytesPerSample, int frameStride )
{
	Mutex::Autolock lock( mLock );

	s_nSampleRate      = sampleRate;
	s_nChannel         = channelNum;
	s_nbytesPerSample  = bytesPerSample;
	s_nStride          = frameStride;

	// bytes per frame
	s_nBytesPerFrame   = s_nSamplesPerFrame * s_nbytesPerSample;

	return OK;
}

// XXX: must be called in callback
status_t CAlsaRecorder::fnGetFrame( char *pDstBuf, int nByteLen )
{
	Mutex::Autolock lock( mLock );

	// LOGE( "s_nRecordByte: %d\n", s_nRecordByte );
	if ( s_nRecordByte < nByteLen )
	{
		// LOGE( "######## underrun ########\n" );
		return -1;
	}

	int read = s_nReadPos;
	int i;
	for ( i = 0; i < nByteLen; i++ )
	{
		pDstBuf[i] = s_pDataBuf[read];
		read       = (read + 1) % s_nDataBufLen;
	}

	s_nReadPos     = (s_nReadPos + s_nStride * s_nbytesPerSample) % s_nDataBufLen;
	s_nRecordByte -= s_nStride * s_nbytesPerSample;

	return OK;
}

// XXX: must be called in callback
status_t CAlsaRecorder::fnClean()
{
	s_nReadPos = s_nWritePos = 0;
	s_nRecordByte = 0;

	return OK;
}

/*  @Description : open the alsa driver and set the cooresponding params
                   start a new thread to get the PCM raw data
    @Parameters  : NULL
    @Return      : status_t
                   0     start alsa driver successfully
                   other fail
    @Note        : N/A
*/
status_t CAlsaRecorder::fnStart()
{
	Mutex::Autolock lock( mLock );

	if ( s_pHandle != NULL )
	{
		return OK;
	}

	int ret;
	int dir;

	// allocate buffer
	s_nDataBufLen = s_nBytesPerFrame * 100; // 100 frames
	s_pDataBuf    = (char*)calloc( s_nDataBufLen, 1 );
	s_pPeriodBuf  = (char*)calloc( s_nBytesPerFrame + 100, 1 );
	s_nRecordByte = 0;
	s_nReadPos    = 0;
	s_nWritePos   = 0;

	/* open PCM device for recording(capture) */
	ret = snd_pcm_open( &s_pHandle, "AndroidCapture_BuiltinMic_normal",  SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC );
	if ( ret < 0 )
	{
		LOGE( "unable to open pcm device: %s\n", snd_strerror( ret ) );
		return ret;
	}
	/* allocate a hardware parameters object */
	snd_pcm_hw_params_alloca( &s_pParams );
	/* fill it in with default values */
	snd_pcm_hw_params_any( s_pHandle, s_pParams );
	/* interleaved mode */
	snd_pcm_hw_params_set_access( s_pHandle, s_pParams, SND_PCM_ACCESS_RW_INTERLEAVED );
	/* signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format( s_pHandle, s_pParams, SND_PCM_FORMAT_S16_LE );
	snd_pcm_hw_params_set_channels( s_pHandle, s_pParams, s_nChannel );
	// LOGD( "snd_pcm_hw_params_set_channels: %d\n", s_nChannel );
	// nDir means the chosen exact value is < = > target by ( -1, 0, 1 )
	snd_pcm_hw_params_set_rate_near( s_pHandle, s_pParams, (unsigned int*)&s_nSampleRate, &dir );
	// LOGD( "snd_pcm_hw_params_set_rate_near: %d, dir: %d\n", s_nSampleRate, dir );
	snd_pcm_hw_params_set_period_size_near( s_pHandle, s_pParams, (snd_pcm_uframes_t*)&s_nSamplesPerFrame, &dir );
	// LOGD( "snd_pcm_hw_params_set_period_near: %d, dir: %d\n", s_nSamplesPerFrame, dir );

	/* write the parameters to the driver */
	ret = snd_pcm_hw_params( s_pHandle, s_pParams );
	if ( ret < 0 )
	{
		LOGE( "unable to set hw parameters: %s\n",  snd_strerror( ret ) );
		return ret;
	}

	snd_pcm_uframes_t actualSamplePerFrame;
	snd_pcm_hw_params_get_period_size( s_pParams, &actualSamplePerFrame, &dir );
	LOGD( "actual alsa output frames per period: %d\n", (int)actualSamplePerFrame );

	snd_pcm_hw_params_get_period_time( s_pParams, (unsigned int*)&ret, &dir );
	LOGD( "actual alsa period time: %d us, dir: %d\n", ret, dir );

#if DEBUG_ALSA_DUMP
	char fname[256] = { 0 };
	snprintf( fname, 255, "/sdcard/alsadump%d.pcm", alsaDumpNum );
	alsaDumpNum++;
	alsaDumpFp = fopen( fname, "wb" );
#endif

	mRecordThread = new RecordThread( this );

	return OK;
}

status_t CAlsaRecorder::fnStop()
{
	// Mutex::Autolock lock( mLock );

	if ( !s_pHandle )
	{
		return OK;
	}

	status_t ret;

	ret = mRecordThread->requestExitAndWait();
	if ( ret == WOULD_BLOCK )
	{
		mRecordThread->requestExit();
	}
	mRecordThread.clear();

	// close alsa device
	snd_pcm_drain( s_pHandle );
	usleep( 1000 );
	snd_pcm_close( s_pHandle );
	s_pHandle = NULL;

#if DEBUG_ALSA_DUMP
	fclose( alsaDumpFp );
	alsaDumpFp = NULL;
#endif

	free( s_pDataBuf );
	s_pDataBuf = NULL;
	free( s_pPeriodBuf );
	s_pPeriodBuf = NULL;

	return OK;
}

}
