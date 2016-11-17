#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Errors.h>

#include "OOBWakeonVoice.h"

using namespace android;
using namespace oobwakeonvoice;

int main( int argc, char *argv[] )
{
	sp<ProcessState> proc( ProcessState::self() );
	sp<IServiceManager> sm = defaultServiceManager();

	int ret = sm->addService( String16( "OOBWakeonVoice" ), new OOBWakeonVoice() );
	if ( ret != NO_ERROR )
	{
		LOGE( "add OOBWakeonVoice service return: %d, pls exit and relaunch", ret );
		exit( -1 );
	}

	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
