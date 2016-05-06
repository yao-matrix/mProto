/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _IOOBWAKEONVOICE_CLIENT_H )
#define _IOOBWAKEONVOICE_CLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

using namespace android;

enum
{
	WAKE_ON_KEYWORD,
	ENROLL_SUCCESS,
	ENROLL_FAIL,
};

class IOOBWakeonVoiceClient: public IInterface
{
public:
	DECLARE_META_INTERFACE( OOBWakeonVoiceClient );
	virtual void notifyCallback( int32_t msgType, int32_t ext1, int32_t ext2 ) = 0;
	void(*fnNotifyCallback)( int32_t msgType, int32_t ext1, int32_t ext2 );
};

class BnOOBWakeonVoiceClient: public BnInterface<IOOBWakeonVoiceClient>
{
public:
	virtual status_t onTransact( uint32_t code, const Parcel &data,
	                             Parcel *reply, uint32_t flags = 0 );
};

#endif // _IOOBWAKEONVOICE_CLIENT_H
