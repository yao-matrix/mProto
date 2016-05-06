/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _OOBWAKEONVOICE_H )
#define _OOBWAKEONVOICE_H

#include <utils/threads.h>
#include <pthread.h>
#include <time.h>
#include "utils/Log.h"

#include "IOOBWakeonVoice.h"
#include "IOOBWakeonVoiceClient.h"

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioConfig.h"
#include "AudioUtil.h"

#include "io/streaming/android/AlsaRecorder.h"

using namespace android;

namespace oobwakeonvoice
{
	class OOBWakeonVoice: public BnOOBWakeonVoice, public IBinder::DeathRecipient
	{
	public:
		OOBWakeonVoice();
		~OOBWakeonVoice();

		void     binderDied( const wp<IBinder>& who );

		status_t fnStartEnroll();

		status_t fnStartListen();
		status_t fnStopListen();

		int fnEnableDebug( int i );
		int fnSetEnrollTimes( int t );
		int fnSetSecurityLevel( int s );

		status_t fnRegisterClient( sp<IOOBWakeonVoiceClient> &c );
	private:
		Mutex  mLock;
	};
};

#endif
