MAUDIO_ROOT := $(call my-dir)

# build speech engine lib
include $(MAUDIO_ROOT)/build/android/Mdroid.mk

# build wake on voice console app
include $(MAUDIO_ROOT)/example/src/console/wov/Android.mk

# build wake on voice android app
include $(MAUDIO_ROOT)/example/src/android/wov/Android.mk

# build oob wake on voice android app
include $(MAUDIO_ROOT)/example/src/android/oobwov/Android.mk

# build speaker verification console app
include $(MAUDIO_ROOT)/example/src/console/sv/Android.mk

# build speaker verification android app
include $(MAUDIO_ROOT)/example/src/android/sv/Android.mk

# build audio recorder instrument app
include $(MAUDIO_ROOT)/example/src/android/audiorecorder/Android.mk
