/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _OOBWAKEONVOICE_CLIENT_H )
#define _OOBWAKEONVOICE_CLIENT_H

#include "IOOBWakeonVoiceClient.h"

using namespace android;

class OOBWakeonVoiceClient: public BnOOBWakeonVoiceClient
{
public:
	OOBWakeonVoiceClient();
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 );
};

#endif // _OOBWAKEONVOICE_CLIENT_H