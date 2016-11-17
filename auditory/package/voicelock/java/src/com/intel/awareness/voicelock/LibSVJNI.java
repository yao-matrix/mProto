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
