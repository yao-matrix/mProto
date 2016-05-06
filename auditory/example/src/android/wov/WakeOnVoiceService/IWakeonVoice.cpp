/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <utils/Errors.h>
#include <utils/Log.h>

#include "IWakeonVoice.h"

namespace wakeonvoice
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

	status_t BnWakeonVoice::onTransact( uint32_t code, const Parcel &data,
	                                    Parcel *reply, uint32_t flags )
	{
		LOGI( "onTransact code: %d", code );
		switch ( code )
		{
		case START_LISTEN:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				reply->writeInt32( this->fnStartListen() );
				return OK;
			}
			break;

		case STOP_LISTEN:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				int ret = this->fnStopListen();
				reply->writeInt32( ret );
				return OK;
			}
			break;

		case START_ENROLL:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				reply->writeInt32( this->fnStartEnroll() );
				return OK;
			}
			break;

		case ENABLE_DEBUG:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				int i = data.readInt32();
				reply->writeInt32( this->fnEnableDebug( i ) );
				return OK;
			}
			break;

		case SET_ENROLLTIMES:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				int t = data.readInt32();
				reply->writeInt32( this->fnSetEnrollTimes( t ) );
				return OK;
			}
			break;

		case SET_SECURITYLEVEL:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				int s = data.readInt32();
				reply->writeInt32( this->fnSetSecurityLevel( s ) );
				return OK;
			}
			break;

		case REGISTER_LISTENER:
			{
				CHECK_INTERFACE( IWakeonVoice, data, reply );
				sp<IBinder> b = data.readStrongBinder();
				sp<IWakeonVoiceClient> c = interface_cast<IWakeonVoiceClient>( b );
				reply->writeInt32( this->fnRegisterClient( c ) );
				return OK;
			}
			break;

		default:
			return -1;
		}
	}

	class BpWakeonVoice: public BpInterface<IWakeonVoice>
	{
	public:
		BpWakeonVoice( const sp<IBinder> &impl )
		  :BpInterface<IWakeonVoice>( impl )
		{
		}

		virtual int fnStartListen()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( START_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStopListen()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( STOP_LISTEN, data, &reply );
			return reply.readInt32();
		}

		virtual int fnStartEnroll()
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			remote()->transact( START_ENROLL, data, &reply );
			return reply.readInt32();
		}

		virtual int fnEnableDebug( int i )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( i );
			remote()->transact( ENABLE_DEBUG, data, &reply );
			return reply.readInt32();
		}

		virtual int fnSetEnrollTimes( int t )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( t );
			remote()->transact( SET_ENROLLTIMES, data, &reply );
			return reply.readInt32();
		}

		virtual int fnSetSecurityLevel( int s )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			data.writeInt32( s );
			remote()->transact( SET_SECURITYLEVEL, data, &reply );
			return reply.readInt32();
		}

		virtual int fnRegisterClient( sp<IWakeonVoiceClient> &c )
		{
			Parcel data, reply;
			data.writeInterfaceToken( IWakeonVoice::getInterfaceDescriptor() );
			data.writeStrongBinder( c->asBinder() );
			remote()->transact( REGISTER_LISTENER, data, &reply );
			return reply.readInt32();
		}

	};

	IMPLEMENT_META_INTERFACE( WakeonVoice, "IWakeonVoice" );
};
