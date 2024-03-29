#if !defined( _IWAKEONVOICE_CLIENT_H )
#define _IWAKEONVOICE_CLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

using namespace android;

enum
{
	WAKE_ON_KEYWORD,
	ENROLL_SUCCESS,
	ENROLL_FAIL,
};

class IWakeonVoiceClient: public IInterface
{
public:
	DECLARE_META_INTERFACE( WakeonVoiceClient );
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 ) = 0;
	void(*fnNotifyCallback)( int32_t msgType, int32_t ext1, int32_t ext2 );
};

class BnWakeonVoiceClient: public BnInterface<IWakeonVoiceClient>
{
public:
	virtual status_t onTransact( uint32_t code, const Parcel &data,
	                             Parcel *reply, uint32_t flags = 0 );
};

#endif // _IWAKEONVOICE_CLIENT_H
