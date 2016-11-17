#undef LOG_TAG
#define LOG_TAG "SpeakerVerificationClientLib"

#include "SpeakerVerificationClientLib.h"
#include <stdio.h>

using namespace speakerverification;
using namespace android;

static sp<ISpeakerVerification> api = NULL;
sp<ISpeakerVerificationClient> client = NULL;


/*  @Description : get the binder reference
    @Parameters  : NULL
    @Return      : int
    @Note        : must called after service_register_callback
*/
int sv_service_init()
{
	LOGD( "%s\n", __FUNCTION__ );
	if ( api != NULL )
	{
		return 0;
	}
	sp<IServiceManager> sm = defaultServiceManager();
	sp<IBinder> b = sm->getService( String16( "SpeakerVerification" ) );
	if ( b == NULL )
	{
		return -1;
	}
	api = interface_cast<ISpeakerVerification>( b );

	if ( client != NULL )
	{
		api->fnRegisterClient( client );
	}
	else
	{
		LOGD( "service_init::client == NULL" );
	}

	return 0;
}

/*  @Description : start
    @Parameters  : NULL
    @Return      : int
    @Note        :
*/
int sv_service_startlisten( char *user )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	if ( api->fnStartListen( user ) == OK )
	{
		return OK;
	}
	else
	{
		return -1;
	}
}

/*  @Description : stop
    @Parameters  : NULL
    @Return      : int
    @Note        :
*/
int sv_service_stoplisten()
{
	LOGD( "%s\n", __FUNCTION__ );

	int ret;

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	ret = api->fnStopListen();

	return ret;
}


int sv_service_startenroll( char *user )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnStartEnroll( user );
}


int sv_service_stopenroll()
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnStopEnroll();
}


/*  @Description : register a call back funtion for client
    @Parameters  : NULL
    @Return      : int
    @Note        :
*/
int sv_service_register_callback( void(*fnCb)(int, int, int) )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( client != NULL )
	{
		LOGD( "client != NULL" );
		return -2;
	}
	client = new SpeakerVerificationClient();
	client->fnNotifyCallback = fnCb;

	return 0;
}

