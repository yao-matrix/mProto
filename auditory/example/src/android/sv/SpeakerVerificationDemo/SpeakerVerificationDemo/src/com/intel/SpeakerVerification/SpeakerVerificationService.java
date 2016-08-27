package com.intel.SpeakerVerification;


import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;

import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.content.Context;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

import java.util.Locale;
import android.speech.tts.TextToSpeech;

public class SpeakerVerificationService extends Service implements ISpeakerVerification
{
	private static final String   TAG             = "SpeakerVerificationService";
	private static TextToSpeech   mTTS;
	private PowerManager          mPM;
	private KeyguardManager	      mKM;
	private KeyguardLock	      mKL;

	private static final int      NOTIFICATION_ID = 1;
	private BroadcastReceiver     mReceiver;
	public static boolean         mIsRunning      = false;
	public static String          mUserName;

	private Handler             mHandler          = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			switch ( msg.what )
			{
			case 0: // verification success
				mTTS.speak( "yes lord", TextToSpeech.QUEUE_FLUSH, null );
				break;

			case 1: // verification fail
				mTTS.speak( "hello intruder", TextToSpeech.QUEUE_FLUSH, null );
				break;

			case 4: // timer interrupt
				break;

			default:
				break;
			}
		}
	};

	@Override
	public void onCreate()
	{
		super.onCreate();

		mIsRunning = true;
		mPM = (PowerManager)getSystemService( POWER_SERVICE );
		mKM = (KeyguardManager)getSystemService( Context.KEYGUARD_SERVICE );
		mKL = mKM.newKeyguardLock( "LOCK_TAG" );

		// tts
		mTTS = new TextToSpeech( this.getApplicationContext(),
		                         new TextToSpeech.OnInitListener()
		                         {
		                            public void onInit( int status )
		                            {
		                                new Thread()
		                                {
		                                    public void run()
		                                    {
		                                        mTTS.setLanguage( Locale.US );
		                                    }
		                                }.start();
		                            }
		                         } );

		IntentFilter filter = new IntentFilter( Intent.ACTION_SCREEN_ON );
		filter.addAction( Intent.ACTION_SCREEN_OFF );
		filter.addAction( Intent.ACTION_USER_PRESENT );
		mReceiver = new MyReceiver();
		registerReceiver( mReceiver, filter );

		Notification notification = new Notification( R.drawable.ic_launcher, getText( R.string.app_name ), System.currentTimeMillis() );
		Intent notificationIntent = new Intent( this, SpeakerVerificationDemoActivity_Verification.class );
		notificationIntent.addFlags( Intent.FLAG_ACTIVITY_SINGLE_TOP );
		PendingIntent pendingIntent = PendingIntent.getActivity( this, 0, notificationIntent, 0 );
		notification.setLatestEventInfo( this, getText( R.string.app_name ), "Select to open activity", pendingIntent );
		startForeground( NOTIFICATION_ID, notification );

		try
		{
			SpeakerVerificationJNI.registerEventHandler( this, 1 );
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType ).sendToTarget();
	}

	@Override
	public IBinder onBind( Intent arg )
	{
		return null;
	}

	@Override
	public void onDestroy()
	{
		unregisterReceiver( mReceiver );
		SpeakerVerificationJNI.unregisterEventHandler( this, 1 );
		mTTS.stop();
		mTTS.shutdown();
		mIsRunning = false;
		super.onDestroy();
	}

	public class MyReceiver extends BroadcastReceiver
	{
		@Override
		public void onReceive( Context context, Intent intent )
		{
			if ( intent.getAction().equals( Intent.ACTION_SCREEN_OFF ) )
			{
				mKL.reenableKeyguard();
			}
			else if ( intent.getAction().equals( Intent.ACTION_SCREEN_ON ) )
			{
				// Log.i( TAG, "ON---ON" );
				SpeakerVerificationJNI.Inst.startlisten( mUserName );
			}
		}
	}
}

