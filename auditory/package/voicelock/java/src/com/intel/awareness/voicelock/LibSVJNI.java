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
 * Create Time: 2013/04/18
 */

package com.intel.awareness.voicelock;

public class LibSVJNI {
    static {
        System.loadLibrary("sv");
    }
    public static final int LIBSV_CB_DEF = 0;
    public static final int LIBSV_CB_LISTEN_SUCCESS = 1;
    public static final int LIBSV_CB_LISTEN_FAILURE = 2;
    public static final int LIBSV_CB_ENROLL_SUCCESS = 3;
    public static final int LIBSV_CB_ENROLL_FAIL = 4;
    public static final int LIBSV_CB_ENROLL_SUFFICIENT = 5;
    public static final int LIBSV_CB_ENROLL_CONFUSED = 6;
    public static final int LIBSV_CB_ENROLL_ABORTED = 7;
    public static final int FULL_LISTEN_FRM_NUM = 64;
    public static final int FULL_ENROLL_FRM_NUM = 1875;

    private int mID;

    public LibSVJNI(int id) {
        mID = id;
    }

    public int id() {
        return mID;
    }

    native int init();

    native int uninit();

    native int start_listen(String user);

    native int stop_listen();

    native int process_listen();

    native int start_enroll(String user);

    native int confirm_enroll();

    native int cancel_enroll();

    native int process_enroll();

    native int push_data(byte[] samples);

    native int post_enroll();

    native int abort_post_enroll();

    native String[] get_user_list();

    native int remove_user(String user);

    native int get_result();

    native int get_state();

    native int get_frm_num();
}
