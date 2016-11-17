# BUILD_MULTI_PREBUILT does not work on NDK 
# so must use seperate PREBUILT_STATIC_LIBRARY blocks for each library

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := ippacmerged
LOCAL_SRC_FILES := lib/libippacmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippccmerged
LOCAL_SRC_FILES := lib/libippccmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippdcmerged
LOCAL_SRC_FILES := lib/libippdcmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippsmerged
LOCAL_SRC_FILES := lib/libippsmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippcore
LOCAL_SRC_FILES := lib/libippcore.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippscemerged
LOCAL_SRC_FILES := lib/libippscemerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippscmerged
LOCAL_SRC_FILES := lib/libippscmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippsremerged
LOCAL_SRC_FILES := lib/libippsremerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippsrmerged
LOCAL_SRC_FILES := lib/libippsrmerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ippsemerged
LOCAL_SRC_FILES := lib/libippsemerged.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := iomp5
LOCAL_SRC_FILES := lib/libiomp5.a
include $(PREBUILT_STATIC_LIBRARY)

