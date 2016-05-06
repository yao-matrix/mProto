LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
                  main.cpp \
                  perf_dtw_mfcc.cpp \
                  perf_dtw_mfcc_vad.cpp \
                  perf_dtw_mfcc_multienroll_construct.cpp \
                  perf_dtw_mfcc_frameadmission.cpp \
                  perf_dtw_mfcc_multienroll_naive.cpp \
                  perf_hmm_mfcc.cpp \
                  perf_gmm_mfcc.cpp \
                  test_hmm_mfcc.cpp \
                  test_dtw_mfcc.cpp \
                  test_dtw_mfcc_cascade.cpp \
                  test_dtw_tfi.cpp \
                  ../benchmark/benchmark.cpp \
                  ../benchmark/det.cpp \


LOCAL_STATIC_LIBRARIES += libaudiorecog \

LOCAL_SHARED_LIBRARIES += libcutils \
                          libutils \

LOCAL_MODULE:= WakeonVoice
LOCAL_MODULE_TAGS:=eng

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_C_INCLUDES := $(MAUDIO_ROOT)/example/src/console/benchmark \
                    $(MAUDIO_ROOT)/include \
                    $(MAUDIO_ROOT)/utils \

LOCAL_CFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

