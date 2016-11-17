#if !defined( _WAKEONVOICE_CLIENT_H )
#define _WAKEONVOICE_CLIENT_H

#include "IWakeonVoiceClient.h"

using namespace android;

class WakeonVoiceClient: public BnWakeonVoiceClient
{
public:
	WakeonVoiceClient();
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 );
};

#endif // _WAKEONVOICE_CLIENT_H
