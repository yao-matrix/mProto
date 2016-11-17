#if !defined( _IWAKEONVOICE_H )
#define _IWAKEONVOICE_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <IWakeonVoiceClient.h>
using namespace android;

namespace wakeonvoice
{
	class IWakeonVoice: public IInterface
	{
	public:
		DECLARE_META_INTERFACE( WakeonVoice );

		virtual status_t fnStartListen() = 0;
		virtual status_t fnStopListen() = 0;

		virtual status_t fnStartEnroll() = 0;

		virtual status_t fnEnableDebug( int i ) = 0;
		virtual status_t fnSetEnrollTimes( int t ) = 0;
		virtual status_t fnSetSecurityLevel( int s) = 0;

		virtual status_t fnRegisterClient( sp<IWakeonVoiceClient> &c ) = 0;
	};

	class BnWakeonVoice: public BnInterface<IWakeonVoice>
	{
	public:
		virtual status_t onTransact( uint32_t code, const Parcel &data,
		                             Parcel *reply, uint32_t flags = 0 );
	};
}

#endif // _IWAKEONVOICE_H
