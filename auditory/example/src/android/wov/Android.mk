LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

DEMO_ROOT := $(LOCAL_PATH)

include $(DEMO_ROOT)/WakeOnVoiceDemo/WakeOnVoiceDemo/Mdroid.mk
include $(DEMO_ROOT)/WakeOnVoiceDemo/jni/Mdroid.mk

include $(DEMO_ROOT)/WakeOnVoiceService/Mdroid.mk