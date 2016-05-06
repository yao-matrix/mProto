/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _SPEAKERVERIFICATION_H )
#define _SPEAKERVERIFICATION_H

#include <utils/threads.h>
#include <pthread.h>
#include <time.h>
#include "utils/Log.h"

#include "ISpeakerVerification.h"
#include "ISpeakerVerificationClient.h"

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioConfig.h"
#include "AudioUtil.h"

#include "io/streaming/android/AlsaRecorder.h"

using namespace android;

namespace speakerverification
{
	class SpeakerVerification: public BnSpeakerVerification, public IBinder::DeathRecipient
	{
	public:
		SpeakerVerification();
		~SpeakerVerification();

		void     binderDied( const wp<IBinder>& who );

		status_t fnStartEnroll( char *user );
		status_t fnStopEnroll();
		status_t fnStartListen( char *user );
		status_t fnStopListen();

		status_t fnRegisterClient( sp<ISpeakerVerificationClient> &c );
	private:
		Mutex    mLock;
		status_t fnGenerateModel();

		class GenerateModelThread: public Thread
		{
			SpeakerVerification *mHandle;

			public:
			GenerateModelThread( SpeakerVerification *handle ): Thread( false ), mHandle( handle )
			{
			}

			virtual void onFirstRef()
			{
				run( "GenerateModel", PRIORITY_URGENT_DISPLAY );
			}

			virtual bool threadLoop()
			{
				mHandle->fnGenerateModel();
				return false;
			}
		};

	};
};

#endif
