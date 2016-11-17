package com.intel.awareness.voicelock;

import java.nio.ByteBuffer;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.internal.policy.IFaceLockCallback;
import com.android.internal.policy.IFaceLockInterface;

public class VoiceLockService extends Service implements Callback {
    static final String TAG = "VoiceLock";
    private static final String TMP_USER = "test";
    private static final int DEFAULT_AWAKE_INTERVAL_MS = 10000;
    private static final int NO_RESPONSE_DELAY = 6000;
    private static final int SHOW_MSG_DELAY = 1000;
    private static final int DEF_BUF_SIZE = 16000; // 0.5 sec
    private static final int SAMPLE_RATE = 16000; //Hz
    private static final int MAX_PROG = 1000;
    private static final int BUF_SIZE = 512; //Bytes

    private static final int UNLOCK = 0;
    private static final int CANCEL = 1;
    private static final int REPORT_FAILED_ATTEMPT = 2;
    private static final int EXPOSE_FALLBACK = 3;
    private static final int POKE_WAKELOCK = 4;

    private static final int MESSAGE_SETVIEW = 100;
    private static final int MESSAGE_REMOVEVIEW = 101;
    private static final int MESSAGE_CANCEL = 102;
    private static final int MESSAGE_SUCCESS = 103;
    private static final int MESSAGE_FAILED = 104;
    private static final int MESSAGE_TIMEOUT = 105;

    private static final int LOOP_EXIT_NONE = 0;
    private static final int LOOP_EXIT_SUCCESS = 1;
    private static final int LOOP_EXIT_FAILURE = 2;
    private static final int LOOP_EXIT_TIMEOUT = 3;
    private static final int LOOP_EXIT_USER = 4;
    private static final int LOOP_EXIT_SYSTEM = 5;

    final RemoteCallbackList<IFaceLockCallback> mCallbacks = new RemoteCallbackList<IFaceLockCallback>();

    private WindowManager.LayoutParams mLayoutParams;
    private WindowManager mWM;
    private VoiceLockRecorder mRecorder;
    private int mBufSizeInBytes;
    private ByteBuffer mBuf;
    private int mExit = LOOP_EXIT_NONE;
    private int mCurrFrmNum = 0;

    private View mSoundUnlockView;
    private TextView mStatus;
    private TextView mKeepTalking;
    private Context mContext;
    private ImageButton mCancelBtn;
    private ProgressBar mProgBar;

    class VerifierThread extends Thread {
        private LibSVJNI mJNI = new LibSVJNI(1);
        public void run() {
            try {
                if (mJNI.init() != 0) {
                    mExit = LOOP_EXIT_FAILURE;
                    return;
                }
                if (mJNI.start_listen(TMP_USER) != 0) {
                    mExit = LOOP_EXIT_FAILURE;
                    return;
                }
                mRecorder.startRecording();
                while (mExit == LOOP_EXIT_NONE) {
                    int len = mRecorder.read(mBuf, BUF_SIZE);
                    if (len == AudioRecord.ERROR_BAD_VALUE ||
                        len == AudioRecord.ERROR_INVALID_OPERATION) {
                        Log.e(TAG, "AudioRecord read Error");
                        break;
                    }
                    if (mJNI.push_data(mBuf.array()) == 1) {
                        mJNI.process_listen();
                        int frmNum = mJNI.get_frm_num();
                        if (frmNum > mCurrFrmNum) {
                            mCurrFrmNum = frmNum;
                            mProgBar.post(new Runnable() {
                                public void run() {
                                    float r = (float)mCurrFrmNum / LibSVJNI.FULL_LISTEN_FRM_NUM;
                                    mProgBar.setProgress((int)(r * (float)MAX_PROG));
                                }
                            });
                        }
                        if (mJNI.get_result() == LibSVJNI.LIBSV_CB_LISTEN_SUCCESS) {
                            mExit = LOOP_EXIT_SUCCESS;
                        } else if (mJNI.get_result() == LibSVJNI.LIBSV_CB_LISTEN_FAILURE) {
                            mExit = LOOP_EXIT_FAILURE;
                        }
                    }
                    mBuf.clear();
                }
            } catch (Exception exp) {
                exp.printStackTrace();
                mExit = LOOP_EXIT_FAILURE;
            } finally {
                try {
                    mRecorder.stop();
                    mBuf.clear();
                    mJNI.stop_listen();
                    mJNI.uninit();
                    switch (mExit) {
                    case LOOP_EXIT_SUCCESS:
                        mMainThreadHandler.obtainMessage(MESSAGE_SUCCESS).sendToTarget();
                        break;
                    case LOOP_EXIT_FAILURE:
                        mMainThreadHandler.obtainMessage(MESSAGE_FAILED).sendToTarget();
                        break;
                    case LOOP_EXIT_TIMEOUT:
                        mMainThreadHandler.obtainMessage(MESSAGE_TIMEOUT).sendToTarget();
                        break;
                    case LOOP_EXIT_USER:
                    case LOOP_EXIT_SYSTEM:
                        break;
                    }
                    mExit = LOOP_EXIT_NONE;
                } catch (Exception exp) {
                    exp.printStackTrace();
                }
            }
        }
    }
    private VerifierThread mVerifierThread;
    private Handler mMainThreadHandler;
    private Runnable mStopListenRunnable = new Runnable() {
        public void run() {
            mExit = LOOP_EXIT_TIMEOUT;
        }
    };

    private IFaceLockInterface.Stub mBinder = new IFaceLockInterface.Stub() {

        @Override
        public void registerCallback(IFaceLockCallback paramIFaceLockCallback) {
            if (paramIFaceLockCallback != null)
                mCallbacks.register(paramIFaceLockCallback);
        }

        @Override
        public void startUi(IBinder paramIBinder, int paramInt1, int paramInt2,
                int paramInt3, int paramInt4, boolean paramBoolean) {
            if (paramIBinder != null) {
                mLayoutParams.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL
                        | LayoutParams.FLAG_NOT_FOCUSABLE;
                mLayoutParams.type = LayoutParams.TYPE_APPLICATION_PANEL;
                mLayoutParams.gravity = Gravity.LEFT | Gravity.TOP;
                mLayoutParams.token = paramIBinder;
                mLayoutParams.x = paramInt1;
                mLayoutParams.y = paramInt2;
                mLayoutParams.width = paramInt3;
                mLayoutParams.height = paramInt4;
                mLayoutParams.packageName = "VoiceUnlock";
                mMainThreadHandler.obtainMessage(MESSAGE_SETVIEW,
                    mLayoutParams).sendToTarget();
            }
        }

        @Override
        public void stopUi() {
            mMainThreadHandler.obtainMessage(MESSAGE_REMOVEVIEW).sendToTarget();
        }

        @Override
        public void unregisterCallback(IFaceLockCallback paramIFaceLockCallback) {
            if (paramIFaceLockCallback != null)
                mCallbacks.unregister(paramIFaceLockCallback);
        }
    };

    public void doCallback(int paramInt) {
        int i = mCallbacks.beginBroadcast();
        for (int j = 0; j < i; j++) {
            switch (paramInt) {
            case UNLOCK:
                try {
                    ((IFaceLockCallback) mCallbacks.getBroadcastItem(j))
                            .unlock();
                } catch (RemoteException localRemoteException) {
                    Log.e(TAG, "Remote exception during UNLOCK");
                }
                break;
            case CANCEL:
                try {
                    ((IFaceLockCallback) mCallbacks.getBroadcastItem(j))
                            .cancel();
                } catch (RemoteException e) {
                    Log.e(TAG, "Remote exception during CANCEL");
                }
                break;
            case REPORT_FAILED_ATTEMPT:
                try {
                    ((IFaceLockCallback) mCallbacks.getBroadcastItem(j))
                            .reportFailedAttempt();
                } catch (RemoteException e) {
                    Log.e(TAG, "Remote exception during REPORT_FAILED_ATTEMPT");
                }
                break;
            case POKE_WAKELOCK:
                try {
                    ((IFaceLockCallback) mCallbacks.getBroadcastItem(j))
                            .pokeWakelock(DEFAULT_AWAKE_INTERVAL_MS);
                } catch (RemoteException e) {
                    Log.e(TAG, "Remote exception during POKE_WAKELOCK");
                }
                break;
            default:
                break;
            }
        }
        mCallbacks.finishBroadcast();
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return mBinder;
    }

    private void showStartUnlock() {
        String userName = getString(R.string.this_user);
        mStatus.setText(String.format(getString(R.string.unlocking), userName));
        mKeepTalking.setVisibility(View.VISIBLE);
        mSoundUnlockView.invalidate();
        mWM.updateViewLayout(mSoundUnlockView, mLayoutParams);
    }

    private void showUnlockFailed() {
        String userName = getString(R.string.this_user);
        mStatus.setText(String.format(getString(R.string.unlock_denine), userName));
        mKeepTalking.setVisibility(View.INVISIBLE);
        mSoundUnlockView.invalidate();
        mWM.updateViewLayout(mSoundUnlockView, mLayoutParams);
    }

    private void showUnlockSucceed() {
        String userName = getString(R.string.this_user);
        mStatus.setText(String.format(getString(R.string.unlock_accept), userName));
        mKeepTalking.setVisibility(View.INVISIBLE);
        mSoundUnlockView.invalidate();
        mWM.updateViewLayout(mSoundUnlockView, mLayoutParams);
    }

    private void showUnlockTimeout() {
        mStatus.setText(getString(R.string.unlock_fail));
        mKeepTalking.setVisibility(View.INVISIBLE);
        mSoundUnlockView.invalidate();
        mWM.updateViewLayout(mSoundUnlockView, mLayoutParams);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        try {
            mContext = this;
            mMainThreadHandler = new Handler(this);
            mWM = (WindowManager) getApplicationContext()
                    .getSystemService(WINDOW_SERVICE);
            mLayoutParams = new LayoutParams();

            mSoundUnlockView = LayoutInflater.from(mContext).inflate(
                    R.layout.sound_unlock, null);
            //For Klockwork scan
            if (mWM == null || mSoundUnlockView == null)
                return;

            mStatus = (TextView) mSoundUnlockView.findViewById(R.id.voice_unlock_text);
            mKeepTalking = (TextView) mSoundUnlockView
                    .findViewById(R.id.keep_talking);
            mCancelBtn = (ImageButton) mSoundUnlockView.findViewById(R.id.cancel_button);
            mProgBar = (ProgressBar) mSoundUnlockView.findViewById(R.id.progressBar1);

            //For Klockwork Scan
            if (mStatus == null || mKeepTalking == null || mCancelBtn == null || mProgBar == null)
                return;

            mCancelBtn.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    mMainThreadHandler.obtainMessage(MESSAGE_CANCEL)
                    .sendToTarget();
                }
            });
            mProgBar.setMax(MAX_PROG);

            mRecorder = VoiceLockRecorder.obtain();
            if (mRecorder == null) {
                Log.e(TAG, "AudioRecord Initialize Failed");
            }
            mBuf = ByteBuffer.allocateDirect(BUF_SIZE);

            mVerifierThread = new VerifierThread();
            mVerifierThread.setName("verify");
            mVerifierThread.setPriority(Thread.MAX_PRIORITY);
            mCurrFrmNum = 0;
        } catch (Exception exp) {
            exp.printStackTrace();
        }
    }

    @Override
    public void onDestroy() {
        mExit = LOOP_EXIT_NONE;
        mVerifierThread = null;
        mBuf = null;
        mMainThreadHandler = null;
        VoiceLockRecorder.recycle();
        mRecorder = null;
        mCancelBtn.setOnClickListener(null);
        super.onDestroy();
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
        case MESSAGE_SETVIEW: {
            // UI update
            mWM.addView(mSoundUnlockView, mLayoutParams);
            showStartUnlock();
            // Speaker recognition part
            mExit = LOOP_EXIT_NONE;
            mVerifierThread.start();
            // Listen shall stop in NO_RESPONSE_DELAY seconds
            mMainThreadHandler.postDelayed(mStopListenRunnable,
                    NO_RESPONSE_DELAY);
            break;
        }
        case MESSAGE_REMOVEVIEW: {
            mExit = LOOP_EXIT_SYSTEM;
            try {
                mVerifierThread.join();
            } catch (Exception exp) {
                exp.printStackTrace();
            }
            mMainThreadHandler.removeCallbacks(mStopListenRunnable);
            // UI update
            mWM.removeView(mSoundUnlockView);
            break;
        }
        case MESSAGE_CANCEL: {
            doCallback(CANCEL);
            break;
        }
        case MESSAGE_SUCCESS: {
            // UI update
            showUnlockSucceed();
            // Unlock the screen
            mMainThreadHandler.postDelayed(new Runnable() {
                public void run() {
                    doCallback(UNLOCK);
                }
            }, SHOW_MSG_DELAY);
            break;
        }
        case MESSAGE_FAILED: {
            // UI update
            showUnlockFailed();
            // Show fallback pattern 1 second later
            mMainThreadHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    doCallback(CANCEL);
                }
            }, SHOW_MSG_DELAY);
            break;
        }
        case MESSAGE_TIMEOUT: {
            // UI update
            showUnlockTimeout();
            // Show fallback pattern 1 second later
            mMainThreadHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    doCallback(CANCEL);
                }
            }, SHOW_MSG_DELAY);
            break;
        }
        default:
            break;
        }
        return false;
    }
}
