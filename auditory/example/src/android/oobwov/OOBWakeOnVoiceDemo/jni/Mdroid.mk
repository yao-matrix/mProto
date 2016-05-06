LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h/ \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(LOCAL_PATH)/../../OOBWakeOnVoiceService

LOCAL_SRC_FILES := OOBWakeOnVoiceJNI_Native.cpp

LOCAL_SHARED_LIBRARIES := libstlport \
                          libutils \
                          libbinder \
                          libandroid_runtime \

LOCAL_LDLIBS += -ldl -lm -lpthread -lrt

LOCAL_STATIC_LIBRARIES := liboobwakeonvoicecli

LOCAL_MODULE := liboobwakeonvoicejni
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
