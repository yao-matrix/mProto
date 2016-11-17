#!/bin/bash

# remount
	adb root
	adb shell mount -w -o remount -t ext3 /dev/block/mcb1k0p6 /system
	echo

# create folder needed
	#adb shell rm -r /sdcard/wov
	adb shell rm -r /sdcard/sv
	adb shell mkdir /sdcard/wov
	adb shell rm -r /sdcard/wov/gmm_model
	adb shell mkdir /sdcard/wov/gmm_model
	adb shell mkdir /sdcard/wov/keyword
	adb shell mkdir /sdcard/wov/keyword/oob
	adb shell mkdir /sdcard/wov/debug
	adb shell mkdir /sdcard/wov/debug/oob
	adb shell mkdir /sdcard/sv
	adb shell mkdir /sdcard/sv/model
	adb shell mkdir /sdcard/sv/model/speakers

	adb shell mkdir /sdcard/vad_model

# push wake on voice server
	adb push ./wakeonvoice_server /system/bin
	echo

# install wake on voice apk
	adb push ./libwakeonvoicejni.so /system/lib/libwakeonvoicejni.so
	echo
	adb install -r ./WakeOnVoiceDemo.apk
	echo

# push oob wake on voice server
	adb push ./oobwakeonvoice_server /system/bin
	echo

# install oob wake on voice apk
	adb push ./liboobwakeonvoicejni.so /system/lib/liboobwakeonvoicejni.so
	echo
	adb install -r ./package/OOBWakeOnVoiceDemo.apk
	echo

# push speaker verification server
	adb push ./speakerverification_server /system/bin
	echo

# install speaker verification apk
	adb push ./libspeakerverificationjni.so /system/lib/libspeakerverificationjni.so
	echo
	adb install -r ./SpeakerVerificationDemo.apk
	echo

# push VAD model data
	adb push ./vad_model/speech.gmm /sdcard/vad_model/speech.gmm
	adb push ./vad_model/background.gmm /sdcard/vad_model/background.gmm

# push wake on voice model data
	adb push ./wov_model/gmm_model/hellointel-imposter.gmm /sdcard/wov/gmm_model/hellointel-imposter.gmm
	adb push ./wov_model/gmm_model/hellointel.gmm  /sdcard/wov/gmm_model/hellointel.gmm
	adb push ./wov_model/gmm_model/ubm-512.gmm  /sdcard/wov/gmm_model/ubm-512.gmm

# push speaker verification model data
	adb push ./sv_model/ubm-512.gmm /sdcard/sv/model/ubm-512.gmm
	adb push ./sv_model/speakers /sdcard/sv/model/speakers

# install audio recorder instrument app
	#adb install -r ./package/AudioRecorder.apk
	#echo

# launch server
	#adb shell /system/bin/wakeonvoice_server&

# cat wakeonvoice_server into init.rc
	#adb shell /sbin/cat ./package/initrc_append >> /init.rc
	#adb reboot
