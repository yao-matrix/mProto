#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Errors.h>

#include "SpeakerVerification.h"

using namespace android;
using namespace speakerverification;

int main( int argc, char *argv[] )
{
	sp<ProcessState> proc( ProcessState::self() );
	sp<IServiceManager> sm = defaultServiceManager();

	int ret = sm->addService( String16( "SpeakerVerification" ), new SpeakerVerification() );
	if ( ret != NO_ERROR )
	{
		LOGE( "add SpeakerVerification service return: %d, pls exit and relaunch", ret );
		exit( -1 );
	}

	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
