LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h/ \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(LOCAL_PATH)/../../WakeOnVoiceService

LOCAL_SRC_FILES := WakeOnVoiceJNI_Native.cpp

LOCAL_SHARED_LIBRARIES := libstlport \
                          libutils \
                          libbinder \
                          libandroid_runtime \

LOCAL_LDLIBS += -ldl -lm -lpthread -lrt

LOCAL_STATIC_LIBRARIES := libwakeonvoicecli

LOCAL_MODULE := libwakeonvoicejni
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
