#ifndef AUDIOCLIENTLIB
#define AUDIOCLIENTLIB

#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "IWakeonVoice.h"
#include "IWakeonVoiceClient.h"
#include "WakeonVoiceClient.h"

int  wov_service_init();
int  wov_service_startlisten();
int  wov_service_stoplisten();
int  wov_service_startenroll();
int  wov_service_enabledebug( int i );
int  wov_service_setenrolltimes( int t );
int  wov_service_setsecuritylevel( int s );
int  wov_service_registercb( void(*lpfncb)(int, int, int) );
#endif
