package com.intel.WakeOnVoice;

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
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.view.Menu;

public class WakeOnVoiceDemoActivity extends Activity implements IWakeOnVoice
{
	private static final String TAG = "WakeOnVoiceDemoActivity";
	private static final int    DEFAULT_DEBUG_ENABLE   = 0; // off
	private static final int    DEFAULT_ENROLL_TIMES   = 2; // 1 times
	private static final int    DEFAULT_SECURITY_LEVEL = 1; // Medium

	private ToggleButton        mRecognitionBt;
	private Button              mEnrollBt;
	private Button              mPlaybackBt;
	private boolean             mNeedWelcome    = true;
	private AudioTrack          mAudioTrack;
	private int                 mTrackFrameSize = 0;
	private String              mKeywordFile    = "/sdcard/wov/keyword/keyword0.wav";
	private String[]            mDebug          = new String[]{ "Off", "On" };
	private String[]            mEnrollTimes    = new String[]{ "5", "3", "1" };
	private String[]            mSecurityLevel  = new String[]{ "Strict", "Medium", "Weak" };
	private ListView            mChooseListView;
	private ListView            mChooseMax;
	private ListView            mChooseSecLv;
	private ItemOnClick         mItemOnClick    = new ItemOnClick( DEFAULT_DEBUG_ENABLE );
	private TimesOnClick        mTimesOnClick   = new TimesOnClick( DEFAULT_ENROLL_TIMES );
	private LevelOnClick        mLevelOnClick   = new LevelOnClick( DEFAULT_SECURITY_LEVEL );

	private Handler             mHandler        = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			switch ( msg.what )
			{
				case 1: // enroll success
					if ( msg.arg1 == 0 )
					{
						AlertDialog.Builder success = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
						success.setMessage( "Enroll command succeed!" )
								.setPositiveButton( "OK", null );
						AlertDialog suc = success.create();
						suc.show();
						// enable enrollment button
						mEnrollBt.setEnabled( true );
					}
					else if ( msg.arg1 == 1 )
					{
						AlertDialog.Builder needmore = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
						needmore.setMessage( "Enroll more please! Left: " + msg.arg2 )
								.setPositiveButton( "OK", null );
						AlertDialog ned = needmore.create();
						ned.show();
						WakeOnVoiceJNI.Inst.startenroll();
					}
					break;

				case 2: // enroll fail, to find what is the meaning of the combination of arg1 and arg2 , pls refer to WakeonVoice.cpp
					if ( msg.arg1 == 0 && msg.arg2 == 1 )
					{
						AlertDialog.Builder tooShort = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
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

		final Intent serviceIntent = new Intent( this, WakeOnVoiceService.class );
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
						AlertDialog.Builder builder = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
						builder.setMessage( "Please wait for enrollment finish before recognition" )
						       .setPositiveButton( "OK", null );
						AlertDialog alert = builder.create();
						alert.show();
						mRecognitionBt.setChecked( false );
						return;
					}

					int ret = WakeOnVoiceService.startListen();
					if ( ret != 0 )
					{
						AlertDialog.Builder alertBuilder = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
						alertBuilder.setMessage( "Audio Hardware IO error, pls reboot your phone to recover" )
						            .setPositiveButton( "OK", null );
						AlertDialog alert = alertBuilder.create();
						alert.show();
					}
				}
				else
				{
					Log.i( TAG, "stop recognition" );
					WakeOnVoiceService.stopListen();
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

					// play back
					mAudioTrack.play();
					byte[]          data       = new byte[mTrackFrameSize];
					FileInputStream in         = null;
					int             writeBytes = 0;
					try {
						in = new FileInputStream( mKeywordFile );
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

					mAudioTrack.stop();

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
						AlertDialog.Builder builder = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
						builder.setMessage( "Please stop recognition first before start enrollment" )
						       .setPositiveButton( "OK", null );
						AlertDialog alert = builder.create();
						alert.show();
						return;
					}

					int ret = WakeOnVoiceJNI.Inst.startenroll();
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
			WakeOnVoiceJNI.registerEventHandler( this, 1 );
			WakeOnVoiceJNI.Inst.initwov();

			WakeOnVoiceJNI.Inst.enableDebug( DEFAULT_DEBUG_ENABLE );
			WakeOnVoiceJNI.Inst.setenrolltimes( Integer.valueOf( mEnrollTimes[DEFAULT_ENROLL_TIMES] ) );
			WakeOnVoiceJNI.Inst.setsecuritylevel( DEFAULT_SECURITY_LEVEL );
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}
	}

	@Override
	public boolean onCreateOptionsMenu( Menu menu )
	{
		super.onCreateOptionsMenu( menu );
		menu.add( 0, 1, 1, "Enable debug" );
		menu.add( 0, 2, 2, "Choose Enroll Times" );
		menu.add( 0, 3, 3, "Set Security Level" );
		return true;
	}

	@Override
	public boolean onOptionsItemSelected( MenuItem item )
	{
		switch ( item.getItemId() )
		{
			case 1:
				AlertDialog chooseEnable = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this ).setTitle( "Choose Enable Function:" )
				                                                                                  .setSingleChoiceItems( mDebug, mItemOnClick.getIndex(), mItemOnClick ).create();
				mChooseListView = chooseEnable.getListView();
				chooseEnable.show();
				break;

			case 2:
				AlertDialog chooseMaxEn = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this ).setTitle( "Choose Enrollment times:" )
				                                                                                 .setSingleChoiceItems( mEnrollTimes, mTimesOnClick.getIndex(), mTimesOnClick ).create();
				mChooseMax = chooseMaxEn.getListView();
				chooseMaxEn.show();
				break;

			case 3:
				AlertDialog chooseSecLv = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this ).setTitle( "Choose Security Level:" )
				                                                                                 .setSingleChoiceItems( mSecurityLevel, mLevelOnClick.getIndex(), mLevelOnClick ).create();
				mChooseSecLv = chooseSecLv.getListView();
				chooseSecLv.show();
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
			Toast.makeText( WakeOnVoiceDemoActivity.this, "You have choosen debug option: " + mDebug[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			WakeOnVoiceJNI.Inst.enableDebug( which );
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

			Toast.makeText( WakeOnVoiceDemoActivity.this, "You have choosen enroll times: " + mEnrollTimes[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			WakeOnVoiceJNI.Inst.setenrolltimes( Integer.valueOf( mEnrollTimes[index] ) );
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

			Toast.makeText( WakeOnVoiceDemoActivity.this, "You have choosen security level: " + mSecurityLevel[index], Toast.LENGTH_SHORT ).show();
			dialog.dismiss();

			WakeOnVoiceJNI.Inst.setsecuritylevel( which );
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
			AlertDialog.Builder welcome = new AlertDialog.Builder( WakeOnVoiceDemoActivity.this );
			welcome.setMessage( "Please try pressing 'Enroll command' button and enroll your keyword( with 3 ~ 5 syllables and no obvious silence between syllables )" )
			       .setPositiveButton( "OK", null );
			AlertDialog wel = welcome.create();
			wel.show();

			mNeedWelcome = false;
		}

		WakeOnVoiceService.mWelcomeBack = false;
	}

	@Override
	protected void onPause()
	{
		Log.i( TAG, "onPause" );

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

		final Intent serviceIntent = new Intent( this, WakeOnVoiceService.class );
		stopService( serviceIntent );

		super.onDestroy();
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType, ext1, ext2 ).sendToTarget();
	}
}
