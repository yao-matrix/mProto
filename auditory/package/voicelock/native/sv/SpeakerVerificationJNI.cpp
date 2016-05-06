/*-------------------------------------------------------------------------
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * This source code and all documentation related to the source code
 * ("Material") contains trade secrets and proprietary and confidential
 * information of Intel and its suppliers and licensors. The Material is
 * deemed highly confidential, and is protected by worldwide copyright and
 * trade secret laws and treaty provisions. No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *-------------------------------------------------------------------------
 * Author: Sun, Taiyi (taiyi.sun@intel.com)
 * Create Time: 2013/04/18
 */

#include <stdlib.h>
#include <jni.h>
#include <utils/Log.h>
#include "SpeakerVerification.h"
#include <map>

//using namespace android;

#undef LOG_TAG
#define LOG_TAG "LibSVJNI"
#define JNI_CLS_NAME "com/intel/awareness/voicelock/LibSVJNI"

static std::map<int, SV*> jmap;

static int get_id(JNIEnv *env, jobject obj) {
    jclass cls = env->GetObjectClass(obj);
    jmethodID mid = env->GetMethodID(cls, "id", "()I");
    jint r = env->CallIntMethod(obj, mid);
    return r;
}

static jint jni_init( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) != jmap.end()) {
        delete jmap[id];
    }
    jmap[id] = new SV();
    return jmap[id]->init();
}

static jint jni_uninit( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->uninit();
}

static jint jni_start_listen( JNIEnv *env, jobject thiz, jstring user )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    const char *_user = env->GetStringUTFChars( user, 0 );
    int res = jmap[id]->start_listen( _user );
    env->ReleaseStringUTFChars( user, _user);
    return res;
}

static jint jni_process_listen( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->process_listen();
}

static jint jni_stop_listen( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->stop_listen();
}

static int jni_start_enroll( JNIEnv *env, jobject thiz, jstring user )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    const char *_user = env->GetStringUTFChars( user, 0 );
    int res = jmap[id]->start_enroll( _user );
    env->ReleaseStringUTFChars( user, _user);
    return res;
}

static jint jni_process_enroll( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->process_enroll();
}

static jint jni_post_enroll( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->post_enroll();
}

static jint jni_abort_post_enroll( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->abort_post_enroll();
}

static jint jni_confirm_enroll( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->confirm_enroll();
}

static jint jni_cancel_enroll( JNIEnv *env, jobject thiz )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->cancel_enroll();
}

static jint jni_push_data( JNIEnv *env, jobject thiz, jbyteArray samples)
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    jboolean isCopy = false;
    jint len = env->GetArrayLength(samples);
    short *_samples = (short*)env->GetByteArrayElements(samples, &isCopy);
    if (isCopy) {
        LOGW("%s: JNI copied the buffer!", __func__);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return -1;
    }
    int res = jmap[id]->push_data(_samples, len >> 1);
    env->ReleaseByteArrayElements(samples, (jbyte*)_samples, JNI_ABORT);
    return res;
}

static jarray jni_get_user_list( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return NULL;
    }
    int num = 0;
    jmap[id]->get_user_list(NULL, &num);
    if (num == 0) {
    }
    char (*names)[LIBSV_USER_NAME_LEN];
    names = (char (*)[LIBSV_USER_NAME_LEN])calloc(num, sizeof(names[0]));
    jmap[id]->get_user_list(names, &num);
    if (names == NULL) {
        return NULL;
    }
    jobjectArray arr = env->NewObjectArray(num,
        env->FindClass( "Ljava/lang/String;" ), NULL);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        free(names);
        return NULL;
    }
    for (int i = 0; i < num; ++i) {
        jstring s = env->NewStringUTF(names[i]);
        if (s == NULL)
            break;
        env->SetObjectArrayElement(arr, i, s);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        free(names);
        return NULL;
    }
    free(names);
    return arr;
}

static jint jni_remove_user( JNIEnv *env, jobject thiz, jstring user )
{
    LOGD( "%s", __FUNCTION__ );
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    const char *_user = env->GetStringUTFChars( user, 0 );
    int res = jmap[id]->remove_user( _user );
    env->ReleaseStringUTFChars( user, _user);
    return res;
}

static jint jni_get_result( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->get_result();
}

static jint jni_get_state( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->get_state();
}

static jint jni_get_frm_num( JNIEnv *env, jobject thiz )
{
    int id = get_id(env, thiz);
    if (jmap.find(id) == jmap.end()) {
        return -1;
    }
    return jmap[id]->get_frm_num();
}

static JNINativeMethod gMethods[] =
{
    { "init",              "()I",                    (void*)jni_init             },
    { "uninit",            "()I",                    (void*)jni_uninit           },
    { "start_listen",      "(Ljava/lang/String;)I",  (void*)jni_start_listen     },
    { "process_listen",    "()I",                    (void*)jni_process_listen   },
    { "stop_listen",       "()I",                    (void*)jni_stop_listen      },
    { "start_enroll",      "(Ljava/lang/String;)I",  (void*)jni_start_enroll     },
    { "process_enroll",    "()I",                    (void*)jni_process_enroll   },
    { "post_enroll",       "()I",                    (void*)jni_post_enroll      },
    { "abort_post_enroll", "()I",                    (void*)jni_abort_post_enroll},
    { "confirm_enroll",    "()I"  ,                  (void*)jni_confirm_enroll   },
    { "cancel_enroll",     "()I"  ,                  (void*)jni_cancel_enroll    },
    { "push_data",         "([B)I",                  (void*)jni_push_data        },
    { "get_user_list",     "()[Ljava/lang/String;",  (void*)jni_get_user_list    },
    { "remove_user",       "(Ljava/lang/String;)I",  (void*)jni_remove_user      },
    { "get_result",        "()I",                    (void*)jni_get_result       },
    { "get_state",         "()I",                    (void*)jni_get_state        },
    { "get_frm_num",       "()I",                    (void*)jni_get_frm_num},
};

jint JNI_OnLoad( JavaVM *vm, void *reserved )
{
    JNIEnv *env = NULL;
    if ( vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK )
    {
        LOGE( "ERROR: GetEnv failed( %s, %d )\n", __FUNCTION__, __LINE__ );
        return -1;
    }
    jclass    cls = env->FindClass( JNI_CLS_NAME );
    if ( env->RegisterNatives(cls, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0 )
    {
        LOGE( "ERROR: register failed\n" );
        return -1;
    }
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
}
