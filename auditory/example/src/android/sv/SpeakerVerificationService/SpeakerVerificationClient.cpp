/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <utils/Errors.h>
#include <utils/Log.h>

#include "SpeakerVerificationClient.h"

using namespace android;

SpeakerVerificationClient::SpeakerVerificationClient()
{

}
void SpeakerVerificationClient::notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 )
{
	// LOGD( "SpeakerVerificationClient::notifyCallback msgType[%d]", msgType );
	switch ( msgType )
	{
	case VERIFICATION_SUCCESS:
	case VERIFICATION_FAILURE:
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



