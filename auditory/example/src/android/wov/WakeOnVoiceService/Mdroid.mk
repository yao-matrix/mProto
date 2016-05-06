LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID

LOCAL_SRC_FILES := \
                   IWakeonVoice.cpp \
                   IWakeonVoiceClient.cpp \
                   main_wovservice.cpp \
                   WakeonVoice.cpp

LOCAL_STATIC_LIBRARIES += libaudiorecog

LOCAL_SHARED_LIBRARIES := libstlport \
                          libandroid_runtime \
                          libcutils \
                          libutils \
                          libbinder \
                          libasound \

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_C_INCLUDES := $(TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(ANDROID_BUILD_TOP)/external/alsa-lib/include \
                    $(MAUDIO_ROOT)/include \
                    $(MAUDIO_ROOT)/utils \


LOCAL_MODULE := wakeonvoice_server
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := WakeonVoiceClientLib.cpp \
                   WakeonVoiceClient.cpp \
                   IWakeonVoice.cpp \
                   IWakeonVoiceClient.cpp

LOCAL_SHARED_LIBRARIES := libstlport \
                          libcutils \
                          libutils \
                          libbinder \
                          libasound

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_C_INCLUDES := $(TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(ANDROID_BUILD_TOP)/external/alsa-lib/include \
                    $(MAUDIO_ROOT)/include \
                    $(MAUDIO_ROOT)/utils \

LOCAL_MODULE := libwakeonvoicecli
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

