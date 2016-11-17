package com.intel.WakeOnVoice;


import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
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

public class WakeOnVoiceService extends Service implements IWakeOnVoice
{
	private static final String   TAG             = "WakeOnVoiceService";
	private static TextToSpeech   mTTS;
	private PowerManager          mPM;
	private KeyguardManager	      mKM;
	private PowerManager.WakeLock mWL;
	private KeyguardLock	      mKL;

	private static final int        NOTIFICATION_ID = 1;
	private BroadcastReceiver       mReceiver;
	public  static boolean          isRunning       = false;
	volatile public  static boolean mWelcomeBack    = true;
	private Handler                 mHandler        = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			switch ( msg.what )
			{
			case 0: // detected
				if ( mPM.isScreenOn() )
				{
					Log.i( TAG, "Screen is On" );

					if ( mWelcomeBack )
					{
						mWL.acquire( 5000 );

						mKL.disableKeyguard();

						mTTS.speak( "Yes sir, welcome back!", TextToSpeech.QUEUE_FLUSH, null );
						mWelcomeBack = false;
					}
					else
					{
						mTTS.speak( "Yes sir!", TextToSpeech.QUEUE_FLUSH, null );
					}
				}
				else
				{
					Log.i( TAG, "Screen is Off" );

					mWL.acquire( 5000 );

					mKL.disableKeyguard();
					mTTS.speak( "Yes sir, welcome back!", TextToSpeech.QUEUE_FLUSH, null );

					mWelcomeBack = false;
				}
				break;

			default:
				break;
			}
		}
	};

	public class WOVBinder extends Binder
	{
		WakeOnVoiceService getService()
		{
			return WakeOnVoiceService.this;
		}
	}

	@Override
	public void onCreate()
	{
		super.onCreate();

		isRunning = true;
		mPM = (PowerManager)getSystemService( POWER_SERVICE );
		mWL = mPM.newWakeLock( PowerManager.ACQUIRE_CAUSES_WAKEUP | PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "bright" );
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

		Notification notification = new Notification( R.drawable.ic_launcher_wakeonvoice, getText( R.string.app_name ), System.currentTimeMillis() );
		Intent notificationIntent = new Intent( this, WakeOnVoiceDemoActivity.class );
		notificationIntent.addFlags( Intent.FLAG_ACTIVITY_SINGLE_TOP );
		PendingIntent pendingIntent = PendingIntent.getActivity( this, 0, notificationIntent, 0 );
		notification.setLatestEventInfo( this, getText( R.string.app_name ), "Select to open activity", pendingIntent );
		startForeground( NOTIFICATION_ID, notification );
		try
		{
			WakeOnVoiceJNI.registerEventHandler( this, 0 );
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType, ext1, ext2 ).sendToTarget();

		return;
	}

	public static int startListen()
	{
		int ret = WakeOnVoiceJNI.Inst.startlisten();
		return ret;
	}

	public static int stopListen()
	{
		int ret = WakeOnVoiceJNI.Inst.stoplisten();
		return ret;
	}

	@Override
	public IBinder onBind( Intent arg )
	{
		return null;
	}

	@Override
	public void onDestroy()
	{
		Log.i( TAG, "onDestroy" );

		unregisterReceiver( mReceiver );

		mTTS.stop();
		mTTS.shutdown();
		isRunning = false;

		WakeOnVoiceJNI.Inst.stoplisten();

		super.onDestroy();
	}

	public class MyReceiver extends BroadcastReceiver
	{
		@Override
		public void onReceive( Context context, Intent intent )
		{
			if ( intent.getAction().equals( Intent.ACTION_SCREEN_OFF ) )
			{
				if ( mWL.isHeld() )
				{
					mWL.release();
				}
				mKL.reenableKeyguard();

				mWelcomeBack = true;
			}
			else if ( intent.getAction().equals( Intent.ACTION_SCREEN_ON ) )
			{
				// Log.i( TAG, "ON---ON" );
			}
		}
	}
}

