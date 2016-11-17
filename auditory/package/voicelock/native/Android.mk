LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID -fstack-protector -O2 -D_FORTIFY_SOURCE=2

LOCAL_SRC_FILES := \
                   audiorecog/frontend/vad/vad.cpp \
                   audiorecog/frontend/denoise/denoise.cpp \
                   audiorecog/frontend/preprocess/preprocess.cpp \
                   audiorecog/frontend/window/window.cpp \
                   audiorecog/frontend/feature/mfcc16s.cpp \
                   audiorecog/frontend/feature/fft_based.cpp \
                   audiorecog/ml/gmm/gmm.cpp \
                   utils/math/mmath.cpp \
                   utils/math/dct.cpp \
                   utils/math/fft.cpp \
                   utils/type/matrix.cpp \
                   utils/type/bufring.cpp \
                   utils/io/file/file.cpp \
                   utils/io/file/directory.cpp

LOCAL_C_INCLUDES := \
                    $(ANDROID_BUILD_TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/cv/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/cxcore/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/ml/include \
                    $(LOCAL_PATH)/utils \
                    $(LOCAL_PATH)/include

LOCAL_LDFLAGS += -z relro -z now -fstack-protector

LOCAL_SHARED_LIBRARIES:= \
                      libui \
                      libutils \

LOCAL_MODULE :=libaudiorecog
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)

#################

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID -fstack-protector -O2 -D_FORTIFY_SOURCE=2

LOCAL_SRC_FILES :=  sv/SpeakerVerification.cpp \
                    sv/SpeakerVerificationJNI.cpp \
                    sv/ImposterInfo.cpp

LOCAL_STATIC_LIBRARIES += libaudiorecog

LOCAL_SHARED_LIBRARIES := libstlport \
                          libandroid_runtime \
                          libcutils \
                          libutils

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DRECORD_PCM
endif

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_LDFLAGS += -z relro -z now -fstack-protector

LOCAL_C_INCLUDES := $(ANDROID_BUILD_TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/utils

LOCAL_MODULE := libsv
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

###################

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID

LOCAL_SRC_FILES := sv/main.cpp

LOCAL_STATIC_LIBRARIES += libaudiorecog

LOCAL_SHARED_LIBRARIES := libstlport \
                          libcutils \
                          libutils \
                          libsv

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_C_INCLUDES := $(ANDROID_BUILD_TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/ \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/stl \
                    $(ANDROID_BUILD_TOP)/external/stlport/stlport/using/h \
                    $(ANDROID_BUILD_TOP)/bionic/libstdc++ \
                    $(ANDROID_BUILD_TOP)/bionic \
                    $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/utils

LOCAL_MODULE := libsv_test
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)

