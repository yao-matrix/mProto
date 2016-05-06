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
 * OOBWakeOnVoiceDemoActivity.java
 */

package com.intel.OOBWakeOnVoice;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.view.Menu;

public class OOBWakeOnVoiceDemoActivity extends Activity implements IOOBWakeOnVoice
{
	private static final String TAG = "OOBWakeOnVoiceDemoActivity";
	private static final int    DEFAULT_DEBUG_ENABLE   = 1; // on
	private static final int    DEFAULT_ENROLL_TIMES   = 1; // 3 times
	private static final int    DEFAULT_SECURITY_LEVEL = 1; // Medium

	private ToggleButton        mRecognitionBt;
	private Button              mEnrollBt;
	private Button              mPlaybackBt;
	private boolean             mNeedWelcome        = false;
	private AudioTrack          mAudioTrack;
	private int                 mTrackFrameSize     = 0;
	private String              mKeywordPath        = Environment.getExternalStorageDirectory().getPath() + "/wov/keyword/oob/";
	private String[]            mDebug              = new String[]{ "Off", "On" };
	private String[]            mEnrollTimesSetting = new String[]{ "5", "3" };
	private String[]            mSecurityLevel      = new String[]{ "Strict", "Medium", "Weak" };
	private int                 mEnrollTimes        = Integer.parseInt( mEnrollTimesSetting[DEFAULT_ENROLL_TIMES] );
	private ItemOnClick         mItemOnClick        = new ItemOnClick( DEFAULT_DEBUG_ENABLE );
	private TimesOnClick        mTimesOnClick       = new TimesOnClick( DEFAULT_ENROLL_TIMES );
	private LevelOnClick        mLevelOnClick       = new LevelOnClick( DEFAULT_SECURITY_LEVEL );
	private String              mDebugPath          = Environment.getExternalStorageDirectory().getPath() + "/wov/debug/oob/";

	private Handler             mHandler            = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			switch ( msg.what )
			{
				case 1: // enroll success
					if ( msg.arg1 == 0 )
					{
						AlertDialog.Builder success = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						success.setMessage( "Enroll command succeed!" )
						       .setPositiveButton( "OK", new DialogInterface.OnClickListener()
							                             {
						                                          public void onClick( DialogInterface dialog, int which )
						                                          {
						                                              mEnrollBt.setEnabled( true );
						                                           }
						                                     } );
						AlertDialog suc = success.create();
						suc.show();
					}
					else if ( msg.arg1 == 1 )
					{
						AlertDialog.Builder needmore = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						needmore.setMessage( "Enroll more please! Left: " + msg.arg2 )
						        .setPositiveButton( "OK", new DialogInterface.OnClickListener()
						                                      {
						                                          public void onClick ( DialogInterface dialog, int which )
						                                          {
						                                              OOBWakeOnVoiceJNI.Inst.startenroll();
						                                          }
						                                       } );
						AlertDialog ned = needmore.create();
						ned.show();
					}
					break;

				case 2: // enroll fail, to find what is the meaning of the combination of arg1 and arg2 , pls refer to WakeonVoice.cpp
					if ( msg.arg1 == 0 && msg.arg2 == 1 )
					{
						AlertDialog.Builder tooShort = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						tooShort.setMessage( "Keyword you just enrolled is too short, you need enroll one longer." )
						        .setPositiveButton( "OK", null );
						AlertDialog too = tooShort.create();
						too.show();
						mEnrollBt.setEnabled( true );
					}
					break;

				default:
					break;
			}
		}
	};

	@Override
	public void onCreate( Bundle savedInstanceState )
	{
		Log.i( TAG, "onCreate" );

		super.onCreate( savedInstanceState );
		setContentView( R.layout.main );

		final Intent serviceIntent = new Intent( this, OOBWakeOnVoiceService.class );
		startService( serviceIntent );

		mTrackFrameSize = AudioTrack.getMinBufferSize( 16000, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT );
		mAudioTrack = new AudioTrack( AudioManager.STREAM_MUSIC, 16000, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT,
		                              mTrackFrameSize, AudioTrack.MODE_STREAM );

		mRecognitionBt = (ToggleButton)findViewById( R.id.RecognitionButton );
		mRecognitionBt.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mRecognitionBt.isChecked() )
				{
					Log.i( TAG, "mRecognitionBt.isChecked()" );

					if ( !mEnrollBt.isEnabled() )
					{
						AlertDialog.Builder builder = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						builder.setMessage( "Please wait for enrollment finish before recognition" )
						       .setPositiveButton( "OK", null );
						AlertDialog alert = builder.create();
						alert.show();
						mRecognitionBt.setChecked( false );
						return;
					}

					int ret = OOBWakeOnVoiceJNI.Inst.startlisten();
					if ( ret != 0 )
					{
						AlertDialog.Builder alertBuilder = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						alertBuilder.setMessage( "Audio Hardware IO error, pls reboot your phone to recover" )
						            .setPositiveButton( "OK", null );
						AlertDialog alert = alertBuilder.create();
						alert.show();
					}
				}
				else
				{
					Log.i( TAG, "stop recognition" );

					OOBWakeOnVoiceJNI.Inst.stoplisten();
				}
			}
		} );

		mPlaybackBt = (Button)findViewById( R.id.PlaybackButton );
		mPlaybackBt.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mPlaybackBt.isEnabled() )
				{
					Log.i( TAG, "mPlaybackBt is enabled" );

					// disable play back button while playing back...
					mPlaybackBt.setEnabled( false );
					File      folder   = new File( mKeywordPath );
					String[]  filename = folder.list();
					if ( filename.length == 0 )
					{
						Log.i( TAG, "++++++++++++++++++++You haven't enrolled your voice!++++++++++++++++++++" );
						Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "You haven't enrolled your voice!", Toast.LENGTH_SHORT ).show();
					}
					else
					{
						// play back
						mAudioTrack.play();
						byte[]          data = new byte[mTrackFrameSize];
						FileInputStream in   = null;

						for ( int i = 0; i < mEnrollTimes; i++ )
						{
							int writeBytes = 0;
							try {
								String keywordFile = mKeywordPath + "keyword" + i + ".wav";
								in = new FileInputStream( keywordFile );
								in.skip( 0x2c );
								while ( true )
								{
									if ( ( writeBytes = in.read( data ) ) != -1 )
									{
										// Log.i( TAG, "write " + writeBytes + " bytes" );
										mAudioTrack.write( data, 0, writeBytes ) ;
									}
									else
									{
										break;
									}
								}
							}
							catch ( FileNotFoundException e )
							{
								e.printStackTrace();
							}
							catch ( IOException e )
							{
								e.printStackTrace();
							}

							try
							{
								Thread.sleep( 500 );
							}
							catch ( InterruptedException e )
							{
								e.printStackTrace();
							}
						}
						mAudioTrack.stop();
					}
					// enable play back button
					mPlaybackBt.setEnabled( true );
				}
			}
		} );

		mEnrollBt = (Button)findViewById( R.id.EnrollmentButton );
		mEnrollBt.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mEnrollBt.isEnabled() )
				{
					Log.i( TAG, "mEnrollBt is enabled" );
					if ( mRecognitionBt.isChecked() )
					{
						AlertDialog.Builder builder = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
						builder.setMessage( "Please stop recognition first before start enrollment" )
						       .setPositiveButton( "OK", null );
						AlertDialog alert = builder.create();
						alert.show();
						return;
					}

					int ret = OOBWakeOnVoiceJNI.Inst.startenroll();
					if ( ret != 0 )
					{
						PowerManager pm = (PowerManager)getSystemService( POWER_SERVICE );
						pm.reboot( "hardware error, need reboot" );
					}

					// disable start enroll button while enrolling...
					mEnrollBt.setEnabled( false );
				}
			}
		} );

		try
		{
			Log.i( TAG, "onCreate" );
			OOBWakeOnVoiceJNI.registerEventHandler( this, 1 );
			OOBWakeOnVoiceJNI.Inst.initwov();

			OOBWakeOnVoiceJNI.Inst.enableDebug( DEFAULT_DEBUG_ENABLE );
			OOBWakeOnVoiceJNI.Inst.setenrolltimes( Integer.valueOf( mEnrollTimesSetting[DEFAULT_ENROLL_TIMES] ) );
			OOBWakeOnVoiceJNI.Inst.setsecuritylevel( DEFAULT_SECURITY_LEVEL );
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}

		mNeedWelcome = true;

		Thread tarThread = new Thread()
		{
			public void run()
			{
				try
				{
					Utility.tarAndUploadDir( mDebugPath, "oobwov" );
				}
				catch ( Exception e )
				{
					e.printStackTrace();
				}
			}
		};
		tarThread.start();
	}

	@Override
	public boolean onCreateOptionsMenu( Menu menu )
	{
		super.onCreateOptionsMenu( menu );
		menu.add( 0, 1, 1, "Enable Debug" );
		menu.add( 0, 2, 2, "Choose Enroll Times" );
		menu.add( 0, 3, 3, "Set Security Level" );
		menu.add( 0, 4, 4, "Delete Enrolled Speaker" );
		return true;
	}

	@Override
	public boolean onOptionsItemSelected( MenuItem item )
	{
		switch ( item.getItemId() )
		{
			case 1:
				AlertDialog chooseEnable = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this ).setTitle( "Choose Enable Function:" )
				                                                                                     .setSingleChoiceItems( mDebug, mItemOnClick.getIndex(), mItemOnClick ).create();
				chooseEnable.getListView();
				chooseEnable.show();
				break;

			case 2:
				AlertDialog chooseMaxEn = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this ).setTitle( "Choose Enrollment times:" )
				                                                                                    .setSingleChoiceItems( mEnrollTimesSetting, mTimesOnClick.getIndex(), mTimesOnClick ).create();
				chooseMaxEn.getListView();
				chooseMaxEn.show();
				break;

			case 3:
				AlertDialog chooseSecLv = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this ).setTitle( "Choose Security Level:" )
				                                                                                    .setSingleChoiceItems( mSecurityLevel, mLevelOnClick.getIndex(), mLevelOnClick ).create();
				chooseSecLv.getListView();
				chooseSecLv.show();
				break;

			case 4:
				String speakerModel = Environment.getExternalStorageDirectory().getPath() + "/wov/gmm_model/hellointel-speaker.gmm";
				File file     = new File( speakerModel );
				File waveFile = new File( mKeywordPath );
				try
				{
					if ( file.exists() )
					{
						File[] files = waveFile.listFiles();
						for ( int i = 0; i < files.length; i++ )
						{
							files[i].delete();
							Log.i( TAG, "--------------deleting wave files -------------" );
						}
						boolean d = file.delete();
						if ( d )
						{
							Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "Success! Speaker model deleted", Toast.LENGTH_SHORT ).show();
						}
						else
						{
							Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "Fail! Pls check if this file being using", Toast.LENGTH_SHORT ).show();
						}
					}
					else
					{
						Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "Speaker model doesn't exists", Toast.LENGTH_SHORT ).show();
					}
				}
				catch( Exception e )
				{
					e.printStackTrace();
				}
				break;
		}
		return super.onOptionsItemSelected( item );
	}

	class ItemOnClick implements DialogInterface.OnClickListener
	{
		private int index;

		public ItemOnClick( int index )
		{
			this.index = index;
		}

		public void setIndex( int index )
		{
			this.index = index;
		}

		public int getIndex()
		{
			return index;
		}

		public void onClick( DialogInterface dialog, int which )
		{
			setIndex( which );
			Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "You have choosen debug option: " + mDebug[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			OOBWakeOnVoiceJNI.Inst.enableDebug( which );
		}
	}

	class TimesOnClick implements DialogInterface.OnClickListener
	{
		private int index;

		public TimesOnClick( int index )
		{
			this.index = index;
		}

		public void setIndex( int index )
		{
			this.index = index;
		}

		public int getIndex()
		{
			return index;
		}

		public void onClick( DialogInterface dialog, int which )
		{
			setIndex( which );

			Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "You have choosen enroll times: " + mEnrollTimesSetting[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			mEnrollTimes = Integer.parseInt( mEnrollTimesSetting[index] );

			OOBWakeOnVoiceJNI.Inst.setenrolltimes( mEnrollTimes );
		}
	}

	class LevelOnClick implements DialogInterface.OnClickListener
	{
		private int index;

		public LevelOnClick( int index )
		{
			this.index = index;
		}

		public void setIndex( int index )
		{
			this.index = index;
		}

		public int getIndex()
		{
			return index;
		}

		public void onClick( DialogInterface dialog, int which )
		{
			setIndex( which );

			Toast.makeText( OOBWakeOnVoiceDemoActivity.this, "You have choosen security level: " + mSecurityLevel[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			OOBWakeOnVoiceJNI.Inst.setsecuritylevel( which );
		}
	 }

	protected void onRestart()
	{
		Log.i( TAG, "onRestart" );
		super.onRestart();
	}

	@Override
	protected void onResume()
	{
		Log.i( TAG, "onResume" );

		super.onResume();

		if ( mNeedWelcome )
		{
			AlertDialog.Builder welcome = new AlertDialog.Builder( OOBWakeOnVoiceDemoActivity.this );
			welcome.setMessage( "Hello! My name is Intel and you can call up me by saying \" Hello Intel\". You can also press the \"enroll\" button " +
			                    "and enroll your own \"Hello Intel\" voice, after which I'll only reply you.\n                          Enjoy! " )
			       .setPositiveButton( "OK", null );
			AlertDialog wel = welcome.create();
			wel.show();
		}
	}

	@Override
	protected void onPause()
	{
		Log.i( TAG, "onPause" );

		mNeedWelcome = false;

		super.onPause();
	}

	@Override
	protected void onStop()
	{
		Log.i( TAG, "onStop" );

		super.onStop();
	}

	@Override
	protected void onDestroy()
	{
		Log.i( TAG, "onDestroy" );

		final Intent serviceIntent = new Intent( this, OOBWakeOnVoiceService.class );
		stopService( serviceIntent );

		super.onDestroy();
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType, ext1, ext2 ).sendToTarget();
	}
}
