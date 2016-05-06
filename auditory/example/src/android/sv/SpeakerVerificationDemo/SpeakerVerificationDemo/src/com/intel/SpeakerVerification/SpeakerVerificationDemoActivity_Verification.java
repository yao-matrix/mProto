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
 * SpeakerVerificationDemoActivity_Verification.java
 *
 */

package com.intel.SpeakerVerification;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.content.Intent;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.ToggleButton;
import android.widget.Toast;
import android.view.Gravity;

public class SpeakerVerificationDemoActivity_Verification<StringTokenizer> extends Activity implements ISpeakerVerification
{
	private static final String TAG = "SpeakerVerificationDemoActivity_Verification";
	private ToggleButton        mVerificationButton;
	private Spinner             mUserSelector;
	private ProgressBar         mProBar;
	private Button              mGotoEnrollButton;
	private boolean             mIsListening    = false;
	private boolean             needListenNow   = false;
	private ArrayAdapter        mUserListAdapter;
	private List<String>        mUserList;

	private Handler             mHandler        = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			// Log.i( TAG, "############Message score is: " + msg.arg2 );
			double score = msg.arg2 / 65536.;
			// Log.i( TAG, "############Double verification score is: " + score );
			String scoreString = String.valueOf( score );
			// Log.i( TAG, "############String verification score is: " + scoreString );

			Toast toast = Toast.makeText( getApplicationContext(), "Verification score is: " + scoreString, Toast.LENGTH_SHORT );
			toast.show();

			switch ( msg.what )
			{
				case 0: // verification success
					AlertDialog.Builder success = new AlertDialog.Builder( SpeakerVerificationDemoActivity_Verification.this );
					success.setMessage( "Success!" )
					       .setPositiveButton( "OK", null );
					AlertDialog suc = success.create();
					suc.show();
					break;

				case 1: // verification fail
					AlertDialog.Builder failure = new AlertDialog.Builder( SpeakerVerificationDemoActivity_Verification.this );
					failure.setMessage( "Fail!" )
					       .setPositiveButton( "OK", null );
					AlertDialog fail = failure.create();
					fail.show();

					if ( msg.arg1 == 1 )
					{
						Toast toast2 = Toast.makeText( getApplicationContext(), "Not Enough speech for decision, pls speak more", Toast.LENGTH_SHORT );
						toast2.show();
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
		super.onCreate( savedInstanceState );
		setContentView( R.layout.verification );

		final Intent serviceIntent = new Intent( this, SpeakerVerificationService.class );

		String path = "/sdcard/sv/model/speakers/";
		File f = new File( path );
		File[] files = null;
		if( f.isDirectory() )
		{
			files = f.listFiles();
		}

		mUserList = new ArrayList<String>();
		for ( int i = 0; i < files.length; i++ )
		{
			String tmp = files[i].getName().substring( 0, files[i].getName().lastIndexOf( "." ) );
			mUserList.add( tmp );
		}

		int size = mUserList.size();
		final String[] usersID = (String[])mUserList.toArray( new String[size] );

		startService( serviceIntent );

		SpeakerVerificationService.mUserName = usersID[0];

		mUserSelector = (Spinner)findViewById( R.id.selectuser );
		mUserSelector.setOnItemSelectedListener( new OnItemSelectedListener()
		{
			public void onItemSelected( AdapterView<?> arg0, View arg1, int arg2, long arg3 )
			{
				SpeakerVerificationService.mUserName = usersID[arg2];
				arg0.setVisibility( View.VISIBLE );
			}
			public void onNothingSelected( AdapterView<?> arg0 )
			{
			}
		} );

		mUserListAdapter = new ArrayAdapter<String>( this, android.R.layout.select_dialog_singlechoice, usersID );
		mUserSelector.setAdapter( mUserListAdapter );

		mProBar = (ProgressBar)findViewById( R.id.progressBar );
		mProBar.setVisibility( 4 );

		mVerificationButton = (ToggleButton)findViewById( R.id.VerificationButton );
		mVerificationButton.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mVerificationButton.isChecked() )
				{
					Log.i( TAG, "VerificationButton is enabled" );

					if ( !mIsListening )
					{
						int ret = SpeakerVerificationJNI.Inst.startlisten( SpeakerVerificationService.mUserName );
						if ( ret != 0 )
						{
							PowerManager pm = (PowerManager)getSystemService( POWER_SERVICE );
							pm.reboot( "hardware error, need reboot" );
						}
						mIsListening = true;
						mProBar.setVisibility( 0 );
						mGotoEnrollButton.setEnabled( false );
					}
				}
				else
				{
					Log.i( TAG, "VerificationButton is disabled" );

					if ( mIsListening )
					{
						SpeakerVerificationJNI.Inst.stoplisten();
						mIsListening = false;
						mProBar.setVisibility( 4 );
						mGotoEnrollButton.setEnabled( true );
					}
				}
			}
		} );

		mGotoEnrollButton = (Button)findViewById( R.id.gotoEnrollButton );
		mGotoEnrollButton.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mGotoEnrollButton.isEnabled() )
				{
					Log.i( TAG, "mGotoEnrollButton is enabled" );

					if ( mIsListening )
					{
						SpeakerVerificationJNI.Inst.stoplisten();
						mIsListening = false;
					}

					Intent intent = new Intent();
					intent.setClass( SpeakerVerificationDemoActivity_Verification.this, SpeakerVerificationDemoActivity_Enroll.class );
					startActivity( intent );
					finish();
				}
			}
		} );

		try
		{
			SpeakerVerificationJNI.registerEventHandler( this, 2 );
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}
	}


	@Override
	protected void onRestart()
	{
		super.onRestart();

		if ( needListenNow && !mIsListening )
		{
			SpeakerVerificationJNI.Inst.startlisten( SpeakerVerificationService.mUserName );
			mIsListening  = true;
			needListenNow = false;
		}
	}

	@Override
	protected void onResume()
	{
		super.onResume();
	}

	@Override
	protected void onStop()
	{
		if ( mIsListening )
		{
			SpeakerVerificationJNI.Inst.stoplisten();
			mIsListening   = false;
			needListenNow = true;
		}
		super.onStop();
	}

	@Override
	protected void onDestroy()
	{
		SpeakerVerificationJNI.unregisterEventHandler( this, 2 );

		final Intent serviceIntent = new Intent( this, SpeakerVerificationService.class );
		stopService( serviceIntent );

		super.onDestroy();
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType, ext1, ext2 ).sendToTarget();
	}

}


			