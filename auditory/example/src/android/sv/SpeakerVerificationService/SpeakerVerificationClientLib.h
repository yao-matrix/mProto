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

#include "ISpeakerVerification.h"
#include "ISpeakerVerificationClient.h"
#include "SpeakerVerificationClient.h"

int  sv_service_init();
int  sv_service_startlisten( char* user );
int  sv_service_stoplisten();
int  sv_service_startenroll( char* user );
int  sv_service_stopenroll();
int  sv_service_register_callback( void(*lpfncb)(int, int, int) );
#endif
