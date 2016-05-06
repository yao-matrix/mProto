/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _WAKEONVOICE_H )
#define _WAKEONVOICE_H

#include <utils/threads.h>
#include <pthread.h>
#include <time.h>
#include "utils/Log.h"

#include "IWakeonVoice.h"
#include "IWakeonVoiceClient.h"

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioConfig.h"
#include "AudioUtil.h"

#include "io/streaming/android/AlsaRecorder.h"

using namespace android;

namespace wakeonvoice
{
	class WakeonVoice: public BnWakeonVoice, public IBinder::DeathRecipient
	{
	public:
		WakeonVoice();
		~WakeonVoice();

		void     binderDied( const wp<IBinder>& who );

		status_t fnStartEnroll();

		status_t fnStartListen();
		status_t fnStopListen();

		int fnEnableDebug( int i );
		int fnSetEnrollTimes( int t );
		int fnSetSecurityLevel( int s );

		status_t fnRegisterClient( sp<IWakeonVoiceClient> &c );
	private:
		Mutex  mLock;
	};
};

#endif
