/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _IOOBWAKEONVOICE_H )
#define _IOOBWAKEONVOICE_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <IOOBWakeonVoiceClient.h>
using namespace android;

namespace oobwakeonvoice
{
	class IOOBWakeonVoice: public IInterface
	{
	public:
		DECLARE_META_INTERFACE( OOBWakeonVoice );

		virtual status_t fnStartListen() = 0;
		virtual status_t fnStopListen() = 0;

		virtual status_t fnStartEnroll() = 0;

		virtual status_t fnEnableDebug( int i ) = 0;
		virtual status_t fnSetEnrollTimes( int t ) = 0;
		virtual status_t fnSetSecurityLevel( int s ) = 0;

		virtual status_t fnRegisterClient( sp<IOOBWakeonVoiceClient> &c ) = 0;
	};

	class BnOOBWakeonVoice: public BnInterface<IOOBWakeonVoice>
	{
	public:
		virtual status_t onTransact( uint32_t code, const Parcel &data,
		                             Parcel *reply, uint32_t flags = 0 );
	};
}

#endif // _IOOBWAKEONVOICE_H
