#include <utils/Errors.h>
#include <utils/Log.h>

#include "IOOBWakeonVoice.h"

namespace oobwakeonvoice
{
	enum
	{
		START_LISTEN,
		STOP_LISTEN,
		START_ENROLL,
		STOP_ENROLL,
		ENABLE_DEBUG,
		SET_ENROLLTIMES,
		SET_SECURITYLEVEL,
		REGISTER_LISTENER,
	};

	status_t BnOOBWakeonVoice::onTransact( uint32_t code, const Parcel &data,
	                                       Parcel *reply, uint32_t flags )
	{
		LOGI( "onTransact code: %d", code );
		switch ( code )
		{
		case START_LISTEN:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				reply->writeInt32( this->fnStartListen() );
				return OK;
			}
			break;

		case STOP_LISTEN:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				int ret = this->fnStopListen();
				reply->writeInt32( ret );
				return OK;
			}
			break;

		case START_ENROLL:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				reply->writeInt32( this->fnStartEnroll() );
				return OK;
			}
			break;

		case ENABLE_DEBUG:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				int i = data.readInt32();
				reply->writeInt32( this->fnEnableDebug( i ) );
				return OK;
			}
			break;

		case SET_ENROLLTIMES:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				int t = data.readInt32();
				reply->writeInt32( this->fnSetEnrollTimes( t ) );
				return OK;
			}
			break;

		case SET_SECURITYLEVEL:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				int s = data.readInt32();
				reply->writeInt32( this->fnSetSecurityLevel( s ) );
				return OK;
			}
			break;

		case REGISTER_LISTENER:
			{
				CHECK_INTERFACE( IOOBWakeonVoice, data, reply );
				sp<IBinder> b = data.readStrongBinder();
				sp<IOOBWakeonVoiceClient> c = interface_cast<IOOBWakeonVoiceClient>( b );
				reply->writeInt32( this->fnRegisterClient( c ) );
				return OK;
			}
			break;

		default:
			return -1;
		}
	}

	class BpOOBWakeonVoice: public BpInterface<IOOBWakeonVoice>
	{
	public:
		BpOOBWakeonVoice( const sp<IBinder> &impl )
		  :BpInterface<IOOBWakeonVoice>( impl )
		{
		}

		virtual int fnStartListen()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( START_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStopListen()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( STOP_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStartEnroll()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( START_ENROLL, data, &reply );
			return reply.readInt32();
		}

		virtual int fnEnableDebug( int i )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( i );
			remote()->transact( ENABLE_DEBUG, data, &reply );
			return reply.readInt32();
		}

		virtual int fnSetEnrollTimes( int t )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( t );
			remote()->transact( SET_ENROLLTIMES, data, &reply );
			return reply.readInt32();
		}

		virtual int fnSetSecurityLevel( int s )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( s );
			remote()->transact( SET_SECURITYLEVEL, data, &reply );
			return reply.readInt32();
		}

		virtual int fnRegisterClient( sp<IOOBWakeonVoiceClient> &c )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IOOBWakeonVoice::getInterfaceDescriptor() );
			data.writeStrongBinder( c->asBinder() );
			remote()->transact( REGISTER_LISTENER, data, &reply );
			return reply.readInt32();
		}

	};

	IMPLEMENT_META_INTERFACE( OOBWakeonVoice, "IOOBWakeonVoice" );
};
