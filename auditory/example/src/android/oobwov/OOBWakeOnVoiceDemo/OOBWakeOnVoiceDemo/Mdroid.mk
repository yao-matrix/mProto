LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += $(call all-java-files-under, src/com/intel/OOBWakeOnVoice)

LOCAL_MODULE_TAGS := eng
LOCAL_CERTIFICATE := platform
LOCAL_PACKAGE_NAME := OOBWakeOnVoiceDemo
include $(BUILD_PACKAGE)
