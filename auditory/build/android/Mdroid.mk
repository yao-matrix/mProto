LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID

LOCAL_SRC_FILES := \
                   ../../src/frontend/vad/vad.cpp \
                   ../../src/frontend/denoise/denoise.cpp \
                   ../../src/frontend/preprocess/preprocess.cpp \
                   ../../src/frontend/window/window.cpp \
                   ../../src/frontend/feature/mfcc16s.cpp \
                   ../../src/frontend/feature/fft_based.cpp \
                   ../../src/frontend/feature/pitch.cpp \
                   ../../src/match/dtw.cpp \
                   ../../src/ml/cluster/gmm_cluster.cpp \
                   ../../src/ml/cluster/distance.cpp \
                   ../../src/ml/gmm/gmm.cpp \
                   ../../src/ml/hmm/gmm_hmm.cpp \
                   ../../src/ml/hmm/nrutil.cpp \
                   ../../utils/math/mmath.cpp \
                   ../../utils/math/dct.cpp \
                   ../../utils/math/fft.cpp \
                   ../../utils/type/matrix.cpp \
                   ../../utils/type/bufring.cpp \
                   ../../utils/misc/misc.cpp \
                   ../../utils/io/file/file.cpp \
                   ../../utils/io/file/directory.cpp \
                   ../../utils/io/streaming/android/AlsaRecorder.cpp \


#LOCAL_SRC_FILES := $(call all-cpp-files-under, ../../src ) \
                    $(call all-cpp-files-under, ../../utils )


LOCAL_C_INCLUDES := \
                    $(ANDROID_BUILD_TOP)/frameworks/base/include \
                    $(ANDROID_BUILD_TOP)/external/alsa-lib/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/cv/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/cxcore/include \
                    $(ANDROID_BUILD_TOP)/external/opencv/ml/include \
                    $(MAUDIO_ROOT)/utils \
                    $(MAUDIO_ROOT)/include \

LOCAL_SHARED_LIBRARIES:= \
                      libui \
                      libutils \

LOCAL_MODULE :=libaudiorecog
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
