LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

DEMO_ROOT := $(LOCAL_PATH)

include $(DEMO_ROOT)/OOBWakeOnVoiceDemo/OOBWakeOnVoiceDemo/Mdroid.mk
include $(DEMO_ROOT)/OOBWakeOnVoiceDemo/jni/Mdroid.mk

include $(DEMO_ROOT)/OOBWakeOnVoiceService/Mdroid.mk
