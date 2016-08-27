#if !defined( _ALSARECORDER_H_ )

#define _ALSARECORDER_H_

#define _POSIX_C_SOURCE

#include <utils/threads.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <sys/types.h>

#include "alsa/asoundlib.h"

namespace android
{

class CAlsaRecorder
{
public:

	status_t fnGetFrame( char *pDstBuf, int nByteLen );
	status_t fnStart();
	status_t fnClean();
	status_t fnStop();
	status_t fnRegisterCallback( void(*fnCb)() );
	status_t fnSetParameters( int sampleRate, int channelNum, int bytesPerSample, int frameStride );
	status_t fnCleanBuffer();

	~CAlsaRecorder();

	static CAlsaRecorder* fnGetInstance();
private:

	class RecordThread: public Thread
	{
		CAlsaRecorder *mRecorder;

	public:
		RecordThread( CAlsaRecorder *rec ): Thread( false ), mRecorder( rec )
		{
		}

		virtual void onFirstRef()
		{
			run( "AlsaRecorder", PRIORITY_AUDIO );
		}

		virtual bool threadLoop()
		{
			mRecorder->fnRecordThread();
			return true;
		}
	};

	CAlsaRecorder();

	Mutex                       mLock;

	sp<RecordThread>            mRecordThread;
	void*                       fnRecordThread( );

	static CAlsaRecorder        *s_pAlsaInstance;

	static char                 *s_pDataBuf;
	static int                  s_nDataBufLen;
	static int                  s_nRecordByte;
	static int                  s_nReadPos;
	static int                  s_nWritePos;
	static char                 *s_pPeriodBuf;

	static snd_pcm_t            *s_pHandle;
	static snd_pcm_hw_params_t  *s_pParams;

	static int                  s_nChannel;
	static int                  s_nSamplesPerFrame;
	static int                  s_nBytesPerFrame;
	static int                  s_nSampleRate;
	static int                  s_nbytesPerSample;
	static int                  s_nStride;

	static void (*s_fnCallback)();
};
}

#endif // _ALSARECORDER_H_
