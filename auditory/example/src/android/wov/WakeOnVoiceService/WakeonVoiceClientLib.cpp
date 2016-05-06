/*
 * INTEL CONFIDENTIAL
 * Copyright 2010-2011 Intel Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is owned
 * by Intel Corporation or its suppliers or licensors, and title to such Material
 * remains with Intel Corporation or its suppliers or licensors. The Material contains
 * proprietary information of Intel or its suppliers and licensors. The Material is
 * protected by worldwide copyright laws and treaty provisions. No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed or disclosed in any way without Intel's prior express written permission.
 * No license under any patent, copyright or other intellectual property rights in the
 * Material is granted to or conferred upon you, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.

 * Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any
 * other notice embedded in Materials by Intel or Intel's suppliers or licensors in any way.
 */

/*
 * WakeonVoiceClientLib.cpp
 *
 * Author: qin.duan
 */
#undef LOG_TAG
#define LOG_TAG "WakeonVoiceClientLib"

#include "WakeonVoiceClientLib.h"
#include <stdio.h>

using namespace wakeonvoice;
using namespace android;

static sp<IWakeonVoice> api = NULL;
// void (*server_death_cb)() = NULL;
sp<IWakeonVoiceClient> client = NULL;


/*  @Description : get the binder reference
    @Parameters  : NULL
    @Return      : int
    @Note        : must called after service_register_callback
*/
int wov_service_init()
{
	LOGD( "%s\n", __FUNCTION__ );
	if ( api != NULL )
	{
		return 0;
	}
	sp<IServiceManager> sm = defaultServiceManager();
	sp<IBinder> b = sm->getService( String16("WakeonVoice") );
	if ( b == NULL )
	{
		return -1;
	}
	api = interface_cast<IWakeonVoice>( b );

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
int wov_service_startlisten()
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}
	if ( api->fnStartListen() == OK )
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
int wov_service_stoplisten()
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


int wov_service_startenroll()
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnStartEnroll();
}

int wov_service_enabledebug( int i )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnEnableDebug( i );
}

int wov_service_setenrolltimes( int t )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD( "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnSetEnrollTimes( t );
}

/*  @Description : set security level for recognition
    @Parameters  : s in short of security
    @Return      : int
    @Note        :
*/
int wov_service_setsecuritylevel( int s )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( api == NULL )
	{
		LOGD(  "api == NULL, line: %d\n", __LINE__ );
		return -2;
	}

	return api->fnSetSecurityLevel( s );
}

/*  @Description : register a call back funtion for client
    @Parameters  : NULL
    @Return      : int
    @Note        :
*/
int wov_service_registercb( void(*fnCb)(int, int, int) )
{
	LOGD( "%s\n", __FUNCTION__ );

	if ( client != NULL )
	{
		LOGD( "client != NULL( %s, %s, %d )", __FILE__, __FUNCTION__, __LINE__ );
		client.clear();
	}
	client = new WakeonVoiceClient();

	client->fnNotifyCallback = fnCb;

	return 0;
}

