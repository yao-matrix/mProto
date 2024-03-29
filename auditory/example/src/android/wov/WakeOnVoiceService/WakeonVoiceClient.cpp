#include <utils/Errors.h>
#include <utils/Log.h>

#include "WakeonVoiceClient.h"

using namespace android;

WakeonVoiceClient::WakeonVoiceClient()
{

}
void WakeonVoiceClient::notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 )
{
	// LOGD( "WakeonVoiceClient::notifyCallback msgType[%d]", msgType );
	switch ( msgType )
	{
	case WAKE_ON_KEYWORD:
		fnNotifyCallback( msgType, ext1, ext2 );
		break;

	case ENROLL_SUCCESS:
	case ENROLL_FAIL:
		fnNotifyCallback( msgType, ext1, ext2 );
		break;

	default:
		break;
	}

	return;
}



