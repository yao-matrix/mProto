/*
 * INTEL CONFIDENTIAL
 * Copyright 2010-2011 Intel Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is owned
 * by Intel Corporation or its suppliers or licensors, and title to such Material
 * remains with Intel Corporation or its suppliers or licensors. The Material contains
 * proprietary information of Intel or its suppliers and licensors. The Material is
 * protected by worldwide copyright laws and treaty provisions. No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed or disclosed in any way without Intel's prior express written permission.
 * No license under any patent, copyright or other intellectual property rights in the
 * Material is granted to or conferred upon you, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.

 * Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any
 * other notice embedded in Materials by Intel or Intel's suppliers or licensors in any way.
 */

/*
 * OOBWakeOnVoiceService.java
 */

package com.intel.OOBWakeOnVoice;


import android.app.AlarmManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Environment;
import android.os.FileObserver;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.util.Log;

import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.content.Context;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.TimeZone;
import java.util.TreeSet;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import android.speech.tts.TextToSpeech;

public class OOBWakeOnVoiceService extends Service implements IOOBWakeOnVoice
{
	private static final String   TAG                  = "OOBWakeOnVoiceService";
	private static final int      TARGET_TIME_HOUR     = 9;
	private static final int      TARGET_TIME_MINUTE   = 15;
	private static final int      TARGET_TIME_SECOND   = 0;
	private static final int      NOTIFICATION_ID      = 1;
	private static final String   TIMEUP               = "TimesUp";
	private static final String   ZIPPATH              = Environment.getExternalStorageDirectory().getPath() + "/wov/debug/oob/";

	private static TextToSpeech   mTTS;
	private PowerManager          mPM;
	private KeyguardManager	      mKM;
	private PowerManager.WakeLock mWL;
	private KeyguardLock	      mKL;
	private Context               mContext;
	private AlarmManager          mAlarmManager;
	private PendingIntent         mPITimesToDo;
	private BroadcastReceiver     mReceiver;
	public  static boolean        isRunning            = false;
	private static boolean        mWelcomeBack         = false;

	private Handler               mHandler             = new Handler()
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
						if ( msg.arg1 == 1 )
						{
							mTTS.speak( "Hello, welcome back!", TextToSpeech.QUEUE_FLUSH, null );
						}
						else
						{
							mTTS.speak( "Yes sir, welcome back!", TextToSpeech.QUEUE_FLUSH, null );
						}
						mWelcomeBack = false;
					}
					else
					{
						if ( msg.arg1 == 1 )
						{
							mTTS.speak( "Hello!", TextToSpeech.QUEUE_FLUSH, null );
						}
						else
						{
							mTTS.speak( "Yes sir!", TextToSpeech.QUEUE_FLUSH, null );
						}
					}
				}
				else
				{
					Log.i( TAG, "Screen is Off" );

					mWL.acquire( 5000 );

					mKL.disableKeyguard();
					if ( msg.arg1 == 1 )
					{
						mTTS.speak( "Hello, welcome back!", TextToSpeech.QUEUE_FLUSH, null );
					}
					else
					{
						mTTS.speak( "Yes sir, welcome back!", TextToSpeech.QUEUE_FLUSH, null );
					}
					mWelcomeBack = false;
				}
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

		mContext = this;
		mAlarmManager = (AlarmManager)mContext.getSystemService( Context.ALARM_SERVICE );
		IntentFilter filter = new IntentFilter( Intent.ACTION_SCREEN_ON );
		filter.addAction( Intent.ACTION_SCREEN_OFF );
		filter.addAction( Intent.ACTION_USER_PRESENT );
		filter.addAction( TIMEUP );
		mReceiver = new MyReceiver();
		mContext.registerReceiver( mReceiver, filter );
		Intent timesToDo = new Intent( TIMEUP );
		mPITimesToDo     = PendingIntent.getBroadcast( mContext, 0, timesToDo, 0 );
		mAlarmManager.setRepeating( AlarmManager.RTC_WAKEUP, Utility.setAlarmTime( TARGET_TIME_HOUR, TARGET_TIME_MINUTE, TARGET_TIME_SECOND ), ( 24 * 60 * 60 * 1000 ) , mPITimesToDo );
		Notification notification = new Notification( R.drawable.ic_launcher_wakeonvoice, getText( R.string.app_name ), System.currentTimeMillis() );
		Intent notificationIntent = new Intent( this, OOBWakeOnVoiceDemoActivity.class );
		notificationIntent.addFlags( Intent.FLAG_ACTIVITY_SINGLE_TOP );
		PendingIntent pendingIntent = PendingIntent.getActivity( this, 0, notificationIntent, 0 );
		notification.setLatestEventInfo( this, getText( R.string.app_name ), "Select to open activity", pendingIntent );
		startForeground( NOTIFICATION_ID, notification );
		try
		{
			OOBWakeOnVoiceJNI.registerEventHandler( this, 0 );
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

		OOBWakeOnVoiceJNI.Inst.stoplisten();

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
			else if ( intent.getAction().equals( TIMEUP ) )
			{
				// tar and upload directory
				Thread tarThread = new Thread()
				{
					public void run()
					{
						try
						{
							Utility.tarAndUploadDir( ZIPPATH, "oobwov" );
						}
						catch ( Exception e )
						{
							e.printStackTrace();
						}
					}
				};
				tarThread.start();
			}
		}
	}
}

