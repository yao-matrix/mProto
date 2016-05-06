/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _ISPEAKERVERIFICATION_CLIENT_H )
#define _ISPEAKERVERIFICATION_CLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

using namespace android;

enum
{
	VERIFICATION_SUCCESS,
	VERIFICATION_FAILURE,
	ENROLL_SUCCESS,
	ENROLL_FAIL,
	TIMER_UPDATE,
	ENROLL_SUFFICIENT,
};

class ISpeakerVerificationClient: public IInterface
{
public:
	DECLARE_META_INTERFACE( SpeakerVerificationClient );
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 ) = 0;
	void(*fnNotifyCallback)( int32_t msgType, int32_t ext1, int32_t ext2 );
};

class BnSpeakerVerificationClient: public BnInterface<ISpeakerVerificationClient>
{
public:
	virtual status_t onTransact( uint32_t code, const Parcel &data,
	                             Parcel *reply, uint32_t flags = 0 );
};

#endif // _ISPEAKERVERIFICATION_CLIENT_H
