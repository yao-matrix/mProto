/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <utils/Errors.h>
#include <utils/Log.h>

#include "IWakeonVoiceClient.h"

using namespace android;

enum
{
	NOTIFY_CALLBACK,
};

class BpWakeonVoiceClient: public BpInterface<IWakeonVoiceClient>
{
public:
	BpWakeonVoiceClient( const sp<IBinder> &impl )
	:BpInterface<IWakeonVoiceClient>( impl )
	{
	}

	void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 )
	{
		LOGV( "notifyCallback" );
		Parcel data, reply;
		data.writeInterfaceToken( IWakeonVoiceClient::getInterfaceDescriptor() );
		data.writeInt32( msgType );
		data.writeInt32( ext1 );
		data.writeInt32( ext2 );
		remote()->transact( NOTIFY_CALLBACK, data, &reply, IBinder::FLAG_ONEWAY );
	}
};

IMPLEMENT_META_INTERFACE( WakeonVoiceClient, "IWakeonVoiceClient" );

// -------------------------------------------------------------------------

status_t BnWakeonVoiceClient::onTransact( uint32_t code, const Parcel &data,
                                          Parcel *reply, uint32_t flags )
{
	LOGI( "onTransact code: %d", code );

	switch ( code )
	{
	case NOTIFY_CALLBACK:
		{
			CHECK_INTERFACE( IWakeonVoiceClient, data, reply );
			int32_t msgType = data.readInt32();
			int32_t ext1 = data.readInt32();
			int32_t ext2 = data.readInt32();
			this->notifyCallback( msgType, ext1, ext2 );
			return NO_ERROR;
		}
		break;

	default:
		break;
	}

	return NO_ERROR;
}



