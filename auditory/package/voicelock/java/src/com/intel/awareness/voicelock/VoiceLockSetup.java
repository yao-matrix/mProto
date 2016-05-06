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

import java.util.concurrent.Semaphore;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentSender;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.media.AudioRecord;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import java.nio.ByteBuffer;

public final class VoiceLockSetup extends PreferenceActivity {

    static private final String TAG = "VoiceLock";
    static private final String TMP_USER = "test";
    static private final int DEF_BUF_SIZE = 16000;  //0.5 sec
    static private final int FULL_PROG = 10000;

    public static final int ENROLL_READING = 0;
    public static final int ENROLL_WAITING = 1;
    public static final int ENROLL_SUCCESS = 2;
    public static final int ENROLL_FAILED = 3;

    public Intent getIntent() {
        Intent localIntent = new Intent(super.getIntent());
        localIntent.putExtra(EXTRA_SHOW_FRAGMENT, VoiceLockSetupFragment.class.getName());
        localIntent.putExtra(EXTRA_NO_HEADERS, true);
        return localIntent;
    }

    public static class VoiceLockSetupFragment extends PreferenceFragment implements View.OnClickListener {

        static class ReadingThread extends Thread {
            public static final int LOOPING = 0;
            public static final int FINISH = 1;
            public static final int ABORT = 2;
            public interface Listener {
                void updateState(int state);
                void updateProg(int prog);
                void ask(Object arg);
            }
            private Listener mListener;
            private int mExit;
            private int mCurrFrmNum;
            private ByteBuffer mBuf;
            private VoiceLockRecorder mRecorder;
            private int mBufSizeInBytes;
            private int mState = ENROLL_READING;
            private Semaphore mPauseSem = null;
            private final Semaphore mAnswerSem = new Semaphore(0);
            private Object mAnswer;
            private LibSVJNI mJNI = new LibSVJNI(0);
            public void answer(Object arg) {
                mAnswer = arg;
                mAnswerSem.release();
            }
            static private ReadingThread inst;
            static public synchronized ReadingThread create() {
                if (inst == null)
                    inst = new ReadingThread();
                return inst;
            }
            static public synchronized ReadingThread inst() {
                return inst;
            }
            static public synchronized ReadingThread del() {
                ReadingThread r = inst;
                inst = null;
                return r;
            }

            public void register(Listener lis) {
                mListener = lis;
            }

            public void abortRecord() {
                mExit = ABORT;
                mJNI.abort_post_enroll();
                if (mPauseSem != null) {
                    mPauseSem.release();
                }
            }

            public void pauseRecord() {
                if (mPauseSem != null)
                    return;
                try {
                    mPauseSem = new Semaphore(0);
                } catch (Exception exp) {
                    exp.printStackTrace();
                }
            }

            public void resumeRecord() {
                if (mPauseSem != null) {
                    mPauseSem.release();
                }
            }

            public void requestUpdate() {
                if (mListener != null) {
                    mListener.updateState(mState);
                    mListener.updateProg(mCurrFrmNum);
                }
            }

            private void updateProg(int frm) {
                mCurrFrmNum = frm;
                if (mListener != null)
                    mListener.updateProg(mCurrFrmNum);
            }

            private void updateState(int state) {
                mState = state;
                if (mListener != null)
                    mListener.updateState(mState);
            }

            private boolean init() {
                mBuf = ByteBuffer.allocateDirect(512);
                mRecorder = VoiceLockRecorder.obtain();
                if (mRecorder == null) {
                    Log.e(TAG, "AudioRecord Initialize Failed");
                    return false;
                }
                mExit = LOOPING;
                if (mJNI.init() != 0) {
                    return false;
                }
                return true;
            }

            private void uninit() {
                mJNI.uninit();
                VoiceLockRecorder.recycle();
                mRecorder = null;
                mBuf.clear();
                mBuf = null;
            }

            public void run() {
                try {
                    if (!this.init()) {
                        updateState(ENROLL_FAILED);
                        return;
                    }
                    if (mRecorder == null)
                        return;
                    if (mJNI.start_enroll(TMP_USER) != 0) {
                        updateState(ENROLL_FAILED);
                        return;
                    }
                    mRecorder.startRecording();
                    while (mExit == LOOPING) {
                        int len = mRecorder.read(mBuf, 512);
                        if (len == AudioRecord.ERROR_BAD_VALUE
                                || len == AudioRecord.ERROR_INVALID_OPERATION) {
                            Log.e(TAG, "AudioRecord read Error");
                            updateState(ENROLL_FAILED);
                            mExit = ABORT;
                            break;
                        }
                        if (mJNI.push_data(mBuf.array()) == 1) {
                            if (mJNI.process_enroll() != 0) {
                                updateState(ENROLL_FAILED);
                                mExit = ABORT;
                                break;
                            }
                            int fn = mJNI.get_frm_num();
                            if (fn > mCurrFrmNum) {
                                updateProg(fn);
                            }
                            if (mJNI.get_result() != LibSVJNI.LIBSV_CB_DEF) {
                                mExit = FINISH;
                            }
                        }
                        mBuf.clear();
                        if (mPauseSem != null) {
                            try {
                                mRecorder.stop();
                                mPauseSem.acquire();
                                mPauseSem = null;
                                mRecorder.startRecording();
                            } catch (Exception exp) {
                                exp.printStackTrace();
                            }
                        }
                    }
                    mRecorder.stop();
                    if (mExit == ABORT) {
                        mJNI.cancel_enroll();
                        return;
                    }
                    if (mJNI.get_result() != LibSVJNI.LIBSV_CB_ENROLL_SUFFICIENT) {
                        Log.e(TAG, String.format("Enroll Failed 1 result is %d", mJNI.get_result()));
                        updateState(ENROLL_FAILED);
                    }
                    updateState(ENROLL_WAITING);
                    if (mJNI.post_enroll() != 0) {
                        updateState(ENROLL_FAILED);
                        mJNI.cancel_enroll();
                        return;
                    }
                    if (mExit == ABORT || mJNI.get_result() == LibSVJNI.LIBSV_CB_ENROLL_ABORTED) {
                        mJNI.cancel_enroll();
                        return;
                    }
                    int res = mJNI.get_result();
                    if (res == LibSVJNI.LIBSV_CB_ENROLL_SUCCESS) {
                        //Success
                        mJNI.confirm_enroll();
                        updateState(ENROLL_SUCCESS);
                    } else if (res == LibSVJNI.LIBSV_CB_ENROLL_CONFUSED) {
                        //Confused
                        if (mListener == null) {
                            mJNI.cancel_enroll();
                            updateState(ENROLL_FAILED);
                        }
                        mListener.ask(null);
                        mAnswerSem.acquire();
                        updateState(ENROLL_SUCCESS);
                        if ((Boolean)mAnswer) {
                            mJNI.confirm_enroll();
                        } else {
                            mJNI.cancel_enroll();
                        }
                    } else {
                        //Failed
                        Log.e(TAG, String.format("Enroll Failed 2 result is %d", mJNI.get_result()));
                        updateState(ENROLL_FAILED);
                        mJNI.cancel_enroll();
                    }
                } catch (Exception exp) {
                    exp.printStackTrace();
                    updateState(ENROLL_FAILED);
                    mJNI.cancel_enroll();
                } finally {
                    if (mRecorder != null)
                        mRecorder.stop();
                    this.uninit();
                }
            }
        };

        private Button mBtCancel;
        private Button mBtNext;
        private ProgressBar mProgBar;
        private TextView mTvHeader;
        private TextView mTvRead;
        private TextView mTvMsg;
        private ReadingThread mReadingThread;
        private Handler mHandler;
        private ReadingThread.Listener mListener = new ReadingThread.Listener() {
            @Override
            public void updateState(final int state) {
                mHandler.post(new Runnable() {
                    public void run() {
                        switch (state) {
                        case ENROLL_READING:
                            mTvMsg.setText(R.string.reading);
                            mBtNext.setEnabled(false);
                            mProgBar.setIndeterminate(false);
                            mProgBar.setProgress(0);
                            break;
                        case ENROLL_WAITING:
                            mTvMsg.setText(R.string.waiting);
                            mBtNext.setEnabled(false);
                            mProgBar.setIndeterminate(true);
                            mProgBar.setProgress(FULL_PROG);
                            break;
                        case ENROLL_SUCCESS:
                            mTvMsg.setText(R.string.succ);
                            mBtNext.setEnabled(true);
                            mProgBar.setIndeterminate(false);
                            mProgBar.setProgress(FULL_PROG);
                            break;
                        case ENROLL_FAILED:
                            mTvMsg.setText(R.string.failed);
                            mBtNext.setEnabled(false);
                            mProgBar.setIndeterminate(false);
                            mProgBar.setProgress(0);
                            break;
                        default:
                        }
                    }
                });
            }
            @Override
            public void updateProg(final int frmNum) {
                mHandler.post(new Runnable() {
                    public void run() {
                        int prog = (int)((float)frmNum /
                                (float)LibSVJNI.FULL_ENROLL_FRM_NUM * (float)FULL_PROG);
                        mProgBar.setProgress(prog);
                    }
                });
            }
            @Override
            public void ask(Object arg) {
                mHandler.post(new Runnable() {
                    public void run() {
                        AlertDialog.Builder b = new AlertDialog.Builder(
                                getActivity().getApplicationContext());
                        DialogInterface.OnClickListener click = new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if (which == DialogInterface.BUTTON_POSITIVE) {
                                    mReadingThread.answer(true);
                                    dialog.dismiss();
                                } else if (which == DialogInterface.BUTTON_NEGATIVE) {
                                    mReadingThread.answer(false);
                                    dialog.dismiss();
                                }
                            }
                        };
                        AlertDialog dlg = b.setTitle(R.string.confused)
                            .setCancelable(false)
                            .setPositiveButton(android.R.string.yes, click)
                            .setNegativeButton(android.R.string.no, click)
                            .create();
                        dlg.show();
                    }
                });
            }
        };

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            mHandler = new Handler();
        }

        @Override
        public void onDestroy() {
            mHandler = null;
            super.onDestroy();
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            View root = null;
            try {
                root = inflater.inflate(R.layout.setup_voice_lock, null);

                //For Klockwork Scan
                if (root == null )
                    return root;
                mTvHeader = (TextView) root.findViewById(R.id.headerText);
                mTvRead = (TextView) root.findViewById(R.id.readText);
                mTvMsg = (TextView) root.findViewById(R.id.msgText);
                mBtCancel = (Button) root.findViewById(R.id.footerLeftButton);
                mBtNext = (Button) root.findViewById(R.id.footerRightButton);
                mProgBar = (ProgressBar) root.findViewById(R.id.setup_vl_progbar);

                //For Klockwork Scan
                if (mTvHeader == null || mTvRead == null || mTvMsg == null ||
                    mBtCancel == null || mBtNext == null || mProgBar == null)
                    return root;

                mTvMsg.setText(this.getActivity().getString(R.string.reading));
                mBtCancel.setOnClickListener(this);
                mBtNext.setOnClickListener(this);
                mBtNext.setEnabled(false);
                mBtNext.setTag(mBtNext.getText().toString());
                mProgBar.setMax(FULL_PROG);

                if (savedInstanceState == null) {
                    mReadingThread = ReadingThread.create();
                    mReadingThread.register(mListener);
                    mReadingThread.setPriority(Thread.MAX_PRIORITY);
                    mReadingThread.start();
                } else {
                    mReadingThread = ReadingThread.inst();
                    mReadingThread.register(mListener);
                    mReadingThread.requestUpdate();
                }
            } catch (Exception exp) {
                exp.printStackTrace();
            }
            return root;
        }

        @Override
        public void onDestroyView() {
            mReadingThread.register(null);
            if (getActivity().isFinishing()) {
                mReadingThread.abortRecord();
                try {
                    mReadingThread.join();
                    mReadingThread = null;
                    ReadingThread.del();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            mBtCancel.setOnClickListener(null);
            mBtCancel = null;
            mBtNext.setOnClickListener(null);
            mBtNext = null;
            mTvHeader = null;
            mTvRead = null;
            mTvMsg = null;
            super.onDestroyView();
        }

        @Override
        public void onStart() {
            super.onStart();
            mReadingThread.resumeRecord();
        }

        @Override
        public void onStop() {
            super.onStop();
            mReadingThread.pauseRecord();
        }

        @Override
        public void onClick(View v) {
            if (v == mBtCancel) {
                getActivity().finish();
            } else if (v == mBtNext) {
                PendingIntent localPendingIntent = (PendingIntent) getActivity()
                        .getIntent().getParcelableExtra("PendingIntent");
                try {
                    if (localPendingIntent != null) {
                        getActivity().startIntentSender(
                                localPendingIntent.getIntentSender(), null, 0,
                                0, 0);
                    }
                    getActivity().finish();
                } catch (IntentSender.SendIntentException localSendIntentException) {
                    Log.w(TAG, "Sending pendingIntent failed");
                } catch (Exception exp) {
                    exp.printStackTrace();
                }
            }
        }
    }
}
