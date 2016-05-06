/*-------------------------------------------------------------------------
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * This source code and all documentation related to the source code
 * ("Material") contains trade secrets and proprietary and confidential
 * information of Intel and its suppliers and licensors. The Material is
 * deemed highly confidential, and is protected by worldwide copyright and
 * trade secret laws and treaty provisions. No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *-------------------------------------------------------------------------
 * Author: Sun, Taiyi (taiyi.sun@intel.com)
 * Create Time: 2013/06/21
 */

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
