#ifeq ($(strip $(USE_INTEL_IPP)),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := \
                       lib/libippacmerged.a \
                       lib/libippccmerged.a \
                       lib/libippdcmerged.a \
                       lib/libippsmerged.a \
                       lib/libippcore.a \
                       lib/libippscemerged.a \
                       lib/libippscmerged.a \
                       lib/libippsremerged.a \
                       lib/libippsrmerged.a \
                       lib/libippsemerged.a \
                       lib/libiomp5.a

LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

LOCAL_COPY_HEADERS := \
                     include/ippac.h \
                     include/ippcc.h \
                     include/ippch.h \
                     include/ippcore.h \
                     include/ippcv.h \
                     include/ippdc.h \
                     include/ippdefs.h  \
                     include/ippdi.h \
                     include/ipp.h \
                     include/ippi.h  \
                     include/ippj.h \
                     include/ippm.h \
                     include/ippr.h \
                     include/ipp_s8.h \
                     include/ippsc.h \
                     include/ipps.h \
                     include/ippsr.h \
                     include/ippvc.h \
                     include/ippversion.h \
                     include/ippvm.h
include $(BUILD_COPY_HEADERS)


#endif
