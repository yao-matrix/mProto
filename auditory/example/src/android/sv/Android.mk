LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

DEMO_ROOT := $(LOCAL_PATH)

include $(DEMO_ROOT)/SpeakerVerificationDemo/SpeakerVerificationDemo/Mdroid.mk
include $(DEMO_ROOT)/SpeakerVerificationDemo/jni/Mdroid.mk

include $(DEMO_ROOT)/SpeakerVerificationService/Mdroid.mk
