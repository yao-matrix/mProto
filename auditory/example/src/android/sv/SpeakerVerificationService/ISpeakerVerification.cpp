#include <utils/Errors.h>
#include <utils/Log.h>

#include "ISpeakerVerification.h"

namespace speakerverification
{
	enum
	{
		START_LISTEN,
		STOP_LISTEN,
		START_ENROLL,
		STOP_ENROLL,
		REGISTER_LISTENER,
	};

	status_t BnSpeakerVerification::onTransact( uint32_t code, const Parcel &data,
	                                            Parcel *reply, uint32_t flags )
	{
		LOGI( "onTransact code: %d", code );
		switch ( code )
		{
		case START_LISTEN:
			{
				CHECK_INTERFACE( ISpeakerVerification, data, reply );
				const char* user = data.readCString();

				reply->writeInt32( this->fnStartListen( (char*)user ) );
				return OK;
			}
			break;

		case STOP_LISTEN:
			{
				CHECK_INTERFACE( ISpeakerVerification, data, reply );
				int ret = this->fnStopListen();
				reply->writeInt32( ret );
				return OK;
			}
			break;

		case START_ENROLL:
			{
				CHECK_INTERFACE( ISpeakerVerification, data, reply );
				const char* user = data.readCString();
				reply->writeInt32( this->fnStartEnroll( (char*)user ) );
				return OK;
			}
			break;

		case STOP_ENROLL:
			{
				CHECK_INTERFACE( ISpeakerVerification, data, reply );
				reply->writeInt32( this->fnStopEnroll() );
				return OK;
			}
			break;

		case REGISTER_LISTENER:
			{
				CHECK_INTERFACE( ISpeakerVerification, data, reply );
				sp<IBinder> b = data.readStrongBinder();
				sp<ISpeakerVerificationClient> c = interface_cast<ISpeakerVerificationClient>( b );
				reply->writeInt32( this->fnRegisterClient( c ) );
				return OK;
			}
			break;

		default:
			return -1;
		}
	}

	class BpSpeakerVerification: public BpInterface<ISpeakerVerification>
	{
	public:
		BpSpeakerVerification( const sp<IBinder> &impl )
		  :BpInterface<ISpeakerVerification>( impl )
		{
		}

		virtual int fnStartListen( char *user )
		{
			Parcel data, reply;
			data.writeInterfaceToken( ISpeakerVerification::getInterfaceDescriptor() );
			data.writeCString( (const char*)user );
			remote()->transact( START_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStopListen()
		{
			Parcel data, reply;
			data.writeInterfaceToken( ISpeakerVerification::getInterfaceDescriptor() );
			remote()->transact( STOP_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStartEnroll( char *user )
		{
			Parcel data, reply;
			data.writeInterfaceToken( ISpeakerVerification::getInterfaceDescriptor() );
			data.writeCString( (const char*)user );
			remote()->transact( START_ENROLL, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStopEnroll()
		{
			Parcel data, reply;
			data.writeInterfaceToken( ISpeakerVerification::getInterfaceDescriptor() );
			remote()->transact( STOP_ENROLL, data, &reply );
			return reply.readInt32();
		}
		
		virtual int fnRegisterClient( sp<ISpeakerVerificationClient> &c )
		{
			Parcel data, reply;
			data.writeInterfaceToken( ISpeakerVerification::getInterfaceDescriptor() );
			data.writeStrongBinder( c->asBinder() );
			remote()->transact( REGISTER_LISTENER, data, &reply );
			return reply.readInt32();
		}

	};

	IMPLEMENT_META_INTERFACE( SpeakerVerification, "ISpeakerVerification" );
};
