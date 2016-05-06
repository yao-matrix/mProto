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
 * wake_on_voice_jni.cpp
 * Author: qin.duan
 */

#include <stdlib.h>
#include <jni.h>
#undef  LOG_TAG
#define LOG_TAG "OOBWakeOnVoiceJNI_Native"
#include "utils/Log.h"
#include "OOBWakeonVoiceClientLib.h"
#include "android_runtime/AndroidRuntime.h"

using namespace android;

static JavaVM  *g_vm = NULL;
static jobject g_obj = NULL;
static void    oobwov_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 );

static void oobwov_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 )
{
	JNIEnv    *env = NULL;
	jclass    cls  = NULL;
	jmethodID mid  = NULL;
	bool      isAttached = false;

	if ( g_vm->GetEnv( (void**)&env, JNI_VERSION_1_4 ) != JNI_OK )
	{
		g_vm->AttachCurrentThread( &env, NULL );
		isAttached = true;
	}
	LOGE( "attach state: %d", isAttached );

	cls = env->GetObjectClass( g_obj );

	mid = env->GetMethodID( cls, "NotifyCallback", "(III)V" );

	env->CallVoidMethod( g_obj, mid, msgType, ext1, ext2 );

	if ( isAttached )
	{
		g_vm->DetachCurrentThread();
	}

	return;
}

static int jni_oobwov_init( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	oobwov_service_registercb( oobwov_notifycallback );

	return oobwov_service_init();
}

static int jni_oobwov_startlisten( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	return oobwov_service_startlisten();
}

static int jni_oobwov_stoplisten( JNIEnv *env, jobject thiz )
{
	LOGD( "%s in", __FUNCTION__ );
	int ret;

	ret = oobwov_service_stoplisten();

	// LOGD( "%s out", __FUNCTION__ );
	return ret;
}

static int jni_oobwov_startenroll( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	return oobwov_service_startenroll();
}

static int jni_oobwov_enabledebug ( JNIEnv *env, jobject thiz, jint i )
{
	LOGD( "%s", __FUNCTION__ );
	return oobwov_service_enabledebug( i );
}

static int jni_oobwov_setenrolltimes ( JNIEnv *env, jobject thiz, jint t )
{
	LOGD( "%s", __FUNCTION__ );
	return oobwov_service_setenrolltimes( t );
}

static int jni_oobwov_setsecuritylevel ( JNIEnv *env, jobject thiz, jint s )
{
	LOGD( "%s", __FUNCTION__ );
	return oobwov_service_setsecuritylevel( s );
}

static JNINativeMethod gMethods[] =
{
	{ "oobwov_init",              "()I",  (void*)jni_oobwov_init },
	{ "oobwov_startlisten",       "()I",  (void*)jni_oobwov_startlisten },
	{ "oobwov_stoplisten",        "()I",  (void*)jni_oobwov_stoplisten },
	{ "oobwov_startenroll",       "()I",  (void*)jni_oobwov_startenroll },
	{ "oobwov_enabledebug",       "(I)I", (int*)jni_oobwov_enabledebug },
	{ "oobwov_setenrolltimes",    "(I)I", (int*)jni_oobwov_setenrolltimes },
	{ "oobwov_setsecuritylevel",  "(I)I", (int*)jni_oobwov_setsecuritylevel },
};

jint JNI_OnLoad( JavaVM *vm, void *reserved )
{
	g_vm = vm;
	JNIEnv *env = NULL;
	if ( vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK )
	{
		LOGE( "ERROR: GetEnv failed( %s, %d )\n", __FUNCTION__, __LINE__ );
		return -1;
	}
	if ( AndroidRuntime::registerNativeMethods(env, "com/intel/OOBWakeOnVoice/OOBWakeOnVoiceJNI", gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0 )
	{
		LOGE( "ERROR: register failed\n" );
		return -1;
	}

	jclass    cls = env->FindClass( "com/intel/OOBWakeOnVoice/OOBWakeOnVoiceJNI" );
	jfieldID  fid = env->GetStaticFieldID( cls, "Inst", "Lcom/intel/OOBWakeOnVoice/OOBWakeOnVoiceJNI;" );
	jmethodID mid = env->GetMethodID( cls, "<init>", "()V" );
	jobject   obj = env->NewObject( cls, mid );

	g_obj = env->NewGlobalRef( obj );
	env->SetStaticObjectField( cls, fid, obj );
	return JNI_VERSION_1_4;
}

void JNI_OnUnload( JavaVM *vm, void *reserved )
{
	JNIEnv *env = NULL;
	if ( vm->GetEnv( (void**)&env, JNI_VERSION_1_4 ) != JNI_OK )
	{
		LOGE( "ERROR: GetEnv failed( %s, %s, %d )\n", __FILE__, __FUNCTION__, __LINE__ );
		return;
	}

	env->DeleteGlobalRef( g_obj );
}
