LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform


LOCAL_SRC_FILES := $(call all-java-files-under, src/com/intel)

LOCAL_PACKAGE_NAME := AudioRecorder


include $(BUILD_PACKAGE)

