/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _ISPEAKERVERIFICATION_H )
#define _ISPEAKERVERIFICATION_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <ISpeakerVerificationClient.h>
using namespace android;

namespace speakerverification
{
	class ISpeakerVerification: public IInterface
	{
	public:
		DECLARE_META_INTERFACE( SpeakerVerification );

		virtual status_t fnStartListen( char* ) = 0;
		virtual status_t fnStopListen() = 0;

		virtual status_t fnStartEnroll( char* ) = 0;
		virtual status_t fnStopEnroll() = 0;

		virtual status_t fnRegisterClient( sp<ISpeakerVerificationClient> &c ) = 0;
	};

	class BnSpeakerVerification: public BnInterface<ISpeakerVerification>
	{
	public:
		virtual status_t onTransact( uint32_t code, const Parcel &data,
		                             Parcel *reply, uint32_t flags = 0 );
	};
}

#endif // _ISPEAKERVERIFICATION_H
