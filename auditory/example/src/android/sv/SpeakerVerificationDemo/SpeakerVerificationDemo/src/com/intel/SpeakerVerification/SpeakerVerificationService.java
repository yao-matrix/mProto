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
 * SpeakerVerificationService.java
 *
 */

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

