LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
                  main.cpp \
                  test_pitch_tracking.cpp \
                  test_denoise.cpp \
                  test_vad.cpp \
                  test_vad_gmm.cpp \
                  test_preemphasis.cpp \


LOCAL_STATIC_LIBRARIES += libaudiorecog \

LOCAL_SHARED_LIBRARIES += libcutils \
                          libutils \

LOCAL_MODULE:= TestBench
LOCAL_MODULE_TAGS:=eng

LOCAL_LDLIBS += -ldl -lpthread

LOCAL_C_INCLUDES := $(MAUDIO_ROOT)/example/src/console/benchmark \
                    $(MAUDIO_ROOT)/include \
                    $(MAUDIO_ROOT)/utils \

LOCAL_CFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

