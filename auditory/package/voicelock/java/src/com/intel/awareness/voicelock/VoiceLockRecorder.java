package com.intel.awareness.voicelock;


import android.media.MediaRecorder;
import android.media.AudioRecord;
import android.media.AudioFormat;

public final class VoiceLockRecorder extends AudioRecord {

    static private final int DEF_BUF_SIZE = 16000;  //0.5 sec
    static private VoiceLockRecorder sInst = null;
    static private int sCount = 0;

    static public synchronized VoiceLockRecorder obtain() {
        if (sCount == 0) {
            sInst = new VoiceLockRecorder();
            if (sInst.getState() != AudioRecord.STATE_INITIALIZED) {
                sInst.release();
                sInst = null;
                return null;
            }
        }
        ++sCount;
        return sInst;
    }

    static public synchronized void recycle() {
        if (sCount > 0) {
            --sCount;
            if (sCount == 0) {
                sInst.release();
                sInst = null;
            }
        }
    }

    static private int getBufSize() {
        int size = AudioRecord.getMinBufferSize(
                16000, AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT);
        size = DEF_BUF_SIZE < size ? size : DEF_BUF_SIZE;
        return size;
    }

    private VoiceLockRecorder() {
        super(MediaRecorder.AudioSource.MIC, 16000,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                getBufSize());
    }


}
