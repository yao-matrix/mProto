/*
 * INTEL CONFIDENTIAL
 * Copyright 2010-2011 Intel Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is owned
 * by Intel Corporation or its suppliers or licensors, and title to such Material
 * remains with Intel Corporation or its suppliers or licensors. The Material contains
 * proprietary information of Intel or its suppliers and licensors. The Material is
 * protected by worldwide copyright laws and treaty provisions. No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed or disclosed in any way without Intel's prior express written permission.
 * No license under any patent, copyright or other intellectual property rights in the
 * Material is granted to or conferred upon you, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.

 * Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any
 * other notice embedded in Materials by Intel or Intel's suppliers or licensors in any way.
 */

/*
 * AlsaRecorder.h
 *
 * Author: qin.duan
 *         Yao, Matrix( matrix.yao@intel.com )
 */

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
