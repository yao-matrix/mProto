LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h/ \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(LOCAL_PATH)/../../SpeakerVerificationService

LOCAL_SRC_FILES := SpeakerVerificationJNI_Native.cpp

LOCAL_SHARED_LIBRARIES := libstlport \
                          libutils \
                          libbinder \
                          libandroid_runtime \

LOCAL_LDLIBS += -ldl -lm -lpthread -lrt

LOCAL_STATIC_LIBRARIES := libspeakerverificationcli

LOCAL_MODULE := libspeakerverificationjni
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
