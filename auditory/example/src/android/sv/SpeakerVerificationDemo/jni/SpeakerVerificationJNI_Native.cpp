#include <stdlib.h>
#include <jni.h>
#undef LOG_TAG
#define LOG_TAG "SpeakerVerificationJNI_Native"
#include "utils/Log.h"
#include "SpeakerVerificationClientLib.h"
#include "android_runtime/AndroidRuntime.h"

using namespace android;

static JavaVM  *g_vm = NULL;
static jobject g_obj = NULL;
static void    sv_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 );

static void sv_notifycallback( int32_t msgType, int32_t ext1, int32_t ext2 )
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

static int jni_sv_init( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	sv_service_register_callback( sv_notifycallback );
	return sv_service_init();
}

static int jni_sv_startlisten( JNIEnv *env, jobject thiz, jstring user )
{
	LOGD( "%s", __FUNCTION__ );

	const char *userName = env->GetStringUTFChars( user, 0 );

	return sv_service_startlisten( (char*)userName );
}

static int jni_sv_stoplisten( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );
	int ret;

	ret = sv_service_stoplisten();

	return ret;
}

static int jni_sv_startenroll( JNIEnv *env, jobject thiz, jstring user )
{
	LOGD( "%s", __FUNCTION__ );

	const char *userName = env->GetStringUTFChars( user, 0 );

	return sv_service_startenroll( (char*)userName );
}

static int jni_sv_stopenroll( JNIEnv *env, jobject thiz )
{
	LOGD( "%s", __FUNCTION__ );

	return sv_service_stopenroll();
}

static JNINativeMethod gMethods[] =
{
	{ "sv_init",        "()I",                   (void*)jni_sv_init },
	{ "sv_startlisten", "(Ljava/lang/String;)I", (void*)jni_sv_startlisten },
	{ "sv_stoplisten",  "()I",                   (void*)jni_sv_stoplisten },
	{ "sv_startenroll", "(Ljava/lang/String;)I", (void*)jni_sv_startenroll },
	{ "sv_stopenroll",  "()I",                   (void*)jni_sv_stopenroll },
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
	if ( AndroidRuntime::registerNativeMethods(env, "com/intel/SpeakerVerification/SpeakerVerificationJNI", gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0 )
	{
		LOGE( "ERROR: register failed\n" );
		return -1;
	}

	jclass    cls = env->FindClass( "com/intel/SpeakerVerification/SpeakerVerificationJNI" );
	jfieldID  fid = env->GetStaticFieldID( cls, "Inst", "Lcom/intel/SpeakerVerification/SpeakerVerificationJNI;" );
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
