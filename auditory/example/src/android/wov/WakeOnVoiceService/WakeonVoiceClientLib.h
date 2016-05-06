/*
 * INTEL CONFIDENTIAL
 * Copyright 2010-2011 Intel Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is owned
 * by Intel Corporation or its suppliers or licensors, and title to such Material
 * remains with Intel Corporation or its suppliers or licensors. The Material contains
 * proprietary information of Intel or its suppliers and licensors. The Material is
 * protected by worldwide copyright laws and treaty provisions. No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed or disclosed in any way without Intel's prior express written permission.
 * No license under any patent, copyright or other intellectual property rights in the
 * Material is granted to or conferred upon you, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.

 * Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any
 * other notice embedded in Materials by Intel or Intel's suppliers or licensors in any way.
 */

/*
 * AudioClientLib.h
 *
 * Author: qin.duan
 */

#ifndef AUDIOCLIENTLIB
#define AUDIOCLIENTLIB

#include <utils/RefBase.h>
#include <utils/String16.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "IWakeonVoice.h"
#include "IWakeonVoiceClient.h"
#include "WakeonVoiceClient.h"

int  wov_service_init();
int  wov_service_startlisten();
int  wov_service_stoplisten();
int  wov_service_startenroll();
int  wov_service_enabledebug( int i );
int  wov_service_setenrolltimes( int t );
int  wov_service_setsecuritylevel( int s );
int  wov_service_registercb( void(*lpfncb)(int, int, int) );
#endif
