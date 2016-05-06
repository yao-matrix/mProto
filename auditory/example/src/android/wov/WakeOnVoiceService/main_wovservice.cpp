/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Errors.h>

#include "WakeonVoice.h"

using namespace android;
using namespace wakeonvoice;

int main( int argc, char *argv[] )
{
	sp<ProcessState> proc( ProcessState::self() );
	sp<IServiceManager> sm = defaultServiceManager();

	int ret = sm->addService( String16( "WakeonVoice" ), new WakeonVoice() );
	if ( ret != NO_ERROR )
	{
		LOGE( "add WakeonVoice service return: %d, pls exit and relaunch", ret );
		exit( -1 );
	}

	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
