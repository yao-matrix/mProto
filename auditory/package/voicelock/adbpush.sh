adb remount
adb shell mkdir -p /system/vendor/intel/aware/voicelock
adb push $OUT/system/lib/libsv.so /system/lib/
adb push $OUT/system/bin/libsv_test /system/bin/
adb install -r $OUT/system/app/VoiceLock.apk
adb push ./sdcard /system/vendor/intel/aware/voicelock/
