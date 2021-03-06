#if !defined( _SPEAKERVERIFICATION_CLIENT_H )
#define _SPEAKERVERIFICATION_CLIENT_H

#include "ISpeakerVerificationClient.h"

using namespace android;

class SpeakerVerificationClient: public BnSpeakerVerificationClient
{
public:
	SpeakerVerificationClient();
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 );
};

#endif // _SPEAKERVERIFICATION_CLIENT_H
