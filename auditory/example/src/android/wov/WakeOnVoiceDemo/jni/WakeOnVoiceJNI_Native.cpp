#include <stdlib.h>
#include <jni.h>
#undef  LOG_TAG
#define LOG_TAG "WakeOnVoiceJNI_Native"
#include "utils/Log.h"
#include "WakeonVoiceClientLib.h"
#include "android_runtime/AndroidRuntime.h"

using namespace android;

static JavaVM  *g_vm = NULL;
static jobject g_obj = NULL;
static void    wov_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 );

static void wov_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 )
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

static int jni_wov_init( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	wov_service_registercb( wov_notifycallback );

	return wov_service_init();
}

static int jni_wov_startlisten( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	return wov_service_startlisten();
}

static int jni_wov_stoplisten( JNIEnv *env, jobject thiz )
{
	LOGD( "%s in", __FUNCTION__ );
	int ret;

	ret = wov_service_stoplisten();

	// LOGD( "%s out", __FUNCTION__ );
	return ret;
}

static int jni_wov_startenroll( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	return wov_service_startenroll();
}

static int jni_wov_enabledebug ( JNIEnv *env, jobject thiz, jint i )
{
	LOGD( "%s", __FUNCTION__ );
	return wov_service_enabledebug( i );
}

static int jni_wov_setenrolltimes ( JNIEnv *env, jobject thiz, jint t )
{
	LOGD( "%s", __FUNCTION__ );
	return wov_service_setenrolltimes( t );
}

static int jni_wov_setsecuritylevel ( JNIEnv *env, jobject thiz, jint s )
{
	LOGD( "%s", __FUNCTION__ );
	return wov_service_setsecuritylevel( s );
}

static JNINativeMethod gMethods[] =
{
	{ "wov_init",             "()I",  (void*)jni_wov_init },
	{ "wov_startlisten",      "()I",  (void*)jni_wov_startlisten },
	{ "wov_stoplisten",       "()I",  (void*)jni_wov_stoplisten },
	{ "wov_startenroll",      "()I",  (void*)jni_wov_startenroll },
	{ "wov_enabledebug",      "(I)I", (int*)jni_wov_enabledebug },
	{ "wov_setenrolltimes",   "(I)I", (int*)jni_wov_setenrolltimes },
	{ "wov_setsecuritylevel", "(I)I", (int*)jni_wov_setsecuritylevel },
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
	if ( AndroidRuntime::registerNativeMethods(env, "com/intel/WakeOnVoice/WakeOnVoiceJNI", gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0 )
	{
		LOGE( "ERROR: register failed\n" );
		return -1;
	}

	jclass    cls = env->FindClass( "com/intel/WakeOnVoice/WakeOnVoiceJNI" );
	jfieldID  fid = env->GetStaticFieldID( cls, "Inst", "Lcom/intel/WakeOnVoice/WakeOnVoiceJNI;" );
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
