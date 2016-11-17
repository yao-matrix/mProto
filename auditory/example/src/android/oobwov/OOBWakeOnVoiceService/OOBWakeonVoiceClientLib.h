#ifndef OOBWOVCLIENTLIB
#define OOBWOVCLIENTLIB

#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "IOOBWakeonVoice.h"
#include "IOOBWakeonVoiceClient.h"
#include "OOBWakeonVoiceClient.h"

int  oobwov_service_init();
int  oobwov_service_startlisten();
int  oobwov_service_stoplisten();
int  oobwov_service_startenroll();
int  oobwov_service_enabledebug( int i );
int  oobwov_service_setenrolltimes( int t );
int  oobwov_service_setsecuritylevel( int s );
int  oobwov_service_registercb( void(*lpfncb)(int, int, int) );
#endif
