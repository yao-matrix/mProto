#!/bin/bash

# copy to package directory
	cp $OUT/system/bin/wakeonvoice_server ./package
	cp $OUT/system/lib/libwakeonvoicejni.so ./package
	cp $OUT/system/app/WakeOnVoiceDemo.apk ./package
	cp $OUT/system/bin/oobwakeonvoice_server ./package
	cp $OUT/system/lib/liboobwakeonvoicejni.so ./package
	cp $OUT/system/app/OOBWakeOnVoiceDemo.apk ./package
	cp $OUT/system/bin/speakerverification_server ./package
	cp $OUT/system/lib/libspeakerverificationjni.so ./package
	cp $OUT/system/app/SpeakerVerificationDemo.apk ./package
	cp $OUT/system/app/AudioRecorder.apk ./package
	echo "copy success"

# remount
	adb root
	adb shell mount -w -o remount -t ext3 /dev/block/mcb1k0p6 /system
	echo

# create folder needed
	#adb shell rm -r /sdcard/wov
	#adb shell rm -r /sdcard/sv
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
	adb push ./package/wakeonvoice_server /system/bin
	echo

# install wake on voice apk
	adb push ./package/libwakeonvoicejni.so /system/lib/libwakeonvoicejni.so
	echo
	adb install -r ./package/WakeOnVoiceDemo.apk
	echo

# push oob wake on voice server
	adb push ./package/oobwakeonvoice_server /system/bin
	echo

# install oob wake on voice apk
	adb push ./package/liboobwakeonvoicejni.so /system/lib/liboobwakeonvoicejni.so
	echo
	adb install -r ./package/OOBWakeOnVoiceDemo.apk
	echo

# push speaker verification server
	adb push ./package/speakerverification_server /system/bin
	echo

# install speaker verification apk
	adb push ./package/libspeakerverificationjni.so /system/lib/libspeakerverificationjni.so
	echo
	adb install -r ./package/SpeakerVerificationDemo.apk
	echo

# push VAD model data
	adb push ./package/vad_model/speech.gmm /sdcard/vad_model/speech.gmm
	adb push ./package/vad_model/background.gmm /sdcard/vad_model/background.gmm

# push wake on voice model data
	adb push ./package/wov_model/gmm_model/hellointel-imposter.gmm /sdcard/wov/gmm_model/hellointel-imposter.gmm
	adb push ./package/wov_model/gmm_model/hellointel.gmm  /sdcard/wov/gmm_model/hellointel.gmm
	adb push ./package/wov_model/gmm_model/ubm-512.gmm  /sdcard/wov/gmm_model/ubm-512.gmm

# push speaker verification model data
	adb push ./package/sv_model/ubm-512.gmm /sdcard/sv/model/ubm-512.gmm
	adb push ./package/sv_model/speakers /sdcard/sv/model/speakers

# install audio recorder instrument app
	#adb install -r ./package/AudioRecorder.apk
	#echo

# launch server
	#adb shell /system/bin/wakeonvoice_server&

# cat wakeonvoice_server into init.rc
	#adb shell /sbin/cat ./package/initrc_append >> /init.rc
	#adb reboot
