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
 * SpeakerVerificationDemoActivity_Enroll.java
 *
 */

package com.intel.SpeakerVerification;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;

public class SpeakerVerificationDemoActivity_Enroll extends Activity implements ISpeakerVerification
{
	private static final String TAG = "SpeakerVerificationDemoActivity_Enroll";
	private Button              mStartEnrollmentButton;
	private Button              mStopEnrollmentButton;
	private Button              mGotoVerifyButton;
	private ProgressBar         mProBar;
	private boolean             mNeedWelcome    = false;
	private boolean             mIsEnrolling    = false;

	private String              mUserName       = "";
	private LinearLayout        mRegisterWin;

	private Handler             mHandler        = new Handler()
	{
		@Override
		public void handleMessage( Message msg )
		{
			switch ( msg.what )
			{
				case 2: // enroll success
					AlertDialog.Builder success = new AlertDialog.Builder( SpeakerVerificationDemoActivity_Enroll.this );
					success.setMessage( "Enrollment succeed!" )
					       .setPositiveButton( "OK", null );
					AlertDialog suc = success.create();
					suc.show();

					mProBar.setVisibility( 4 );

					// enable enrollment button
					mStartEnrollmentButton.setEnabled( true );
					mStopEnrollmentButton.setEnabled( true );
					mGotoVerifyButton.setEnabled( true );
					break;

				case 3: // enroll fail
					AlertDialog.Builder failure = new AlertDialog.Builder( SpeakerVerificationDemoActivity_Enroll.this );
					failure.setMessage( "Enrollment failed, you can enroll once again." )
					       .setPositiveButton( "OK", null );
					AlertDialog fail = failure.create();
					fail.show();

					mProBar.setVisibility( 4 );

					// enable enrollment button
					mStartEnrollmentButton.setEnabled( true );
					mStopEnrollmentButton.setEnabled( true );
					mGotoVerifyButton.setEnabled( true );
					break;

				case 5: // enroll sufficient
					mStopEnrollmentButton.setEnabled( false );

					mProBar.setVisibility( 0 );

					SpeakerVerificationJNI.Inst.stopenroll();
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
		setContentView( R.layout.main );

		mStartEnrollmentButton = (Button)findViewById( R.id.EnrollmentButton );
		mStartEnrollmentButton.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mStartEnrollmentButton.isEnabled() )
				{
					Log.i( TAG, "mStartEnrollmentButton is enabled" );

					mRegisterWin = (LinearLayout)getLayoutInflater().inflate( R.layout.enroll_prompt, null );
					new AlertDialog.Builder( SpeakerVerificationDemoActivity_Enroll.this ).setIcon( android.R.drawable.ic_dialog_info )
														.setTitle( "User Register" )
														.setView( mRegisterWin )
														.setPositiveButton( "Confirm",
																new DialogInterface.OnClickListener()
																{
																	public void onClick( DialogInterface dialog, int whichButton )
																	{
																		// handle register process
																		EditText txtName = (EditText)mRegisterWin.findViewById( R.id.userName );
																		mUserName        = txtName.getText().toString();
																		Pattern p = Pattern.compile( "\\s*|\t|\r|\n" );
																		Matcher m = p.matcher( mUserName );
																		mUserName = m.replaceAll( "" );
																		if ( !mUserName.equals("") )
																		{
																			int ret = SpeakerVerificationJNI.Inst.startenroll( mUserName );
																			if ( ret != 0 )
																			{
																				PowerManager pm = (PowerManager)getSystemService( POWER_SERVICE );
																				pm.reboot( "hardware error, need reboot" );
																			}

																			// disable mStartEnrollmentButton while enrolling...
																			mStartEnrollmentButton.setEnabled( false );
																			mGotoVerifyButton.setEnabled( false );
																			mProBar.setVisibility( 0 );
																		}
																	}
																})
														.setNegativeButton("Cancel",
																new DialogInterface.OnClickListener()
																{
																	public void onClick( DialogInterface dialog, int whichButton )
																	{
																	}
																})
														.show();
				}
			}
		} );

		mStopEnrollmentButton = (Button)findViewById( R.id.StopEnrollmentButton );
		mStopEnrollmentButton.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mStopEnrollmentButton.isEnabled() )
				{
					Log.i( TAG, "mStopEnrollmentButton is enabled" );

					mStartEnrollmentButton.setEnabled( false );
					mStopEnrollmentButton.setEnabled( false );

					mProBar.setVisibility( 0 );
				
					SpeakerVerificationJNI.Inst.stopenroll();
				}
			}
		} );

		mProBar = (ProgressBar)findViewById( R.id.progressBar );
		mProBar.setVisibility( 4 );
		
		mGotoVerifyButton = (Button)findViewById( R.id.gotoVerifyButton );
		mGotoVerifyButton.setOnClickListener( new OnClickListener()
		{
			public void onClick( View v )
			{
				if ( mGotoVerifyButton.isEnabled() )
				{
					Log.i( TAG, "mGotoVerifyButton is enabled" );

					if ( mIsEnrolling )
					{
						SpeakerVerificationJNI.Inst.stopenroll();
						mIsEnrolling   = false;
					}
				
					Intent intent = new Intent();
					intent.setClass( SpeakerVerificationDemoActivity_Enroll.this, SpeakerVerificationDemoActivity_Verification.class );
					startActivity( intent );
					finish();
				}
			}
		} );

		
		try
		{
			Log.i( TAG, "onCreate" );
			SpeakerVerificationJNI.registerEventHandler( this, 0 );
			SpeakerVerificationJNI.Inst.initsv();
		}
		catch ( Exception exp )
		{
			Log.i( TAG, exp.toString() );
		}

		mNeedWelcome = true;
	}

	@Override
	protected void onRestart()
	{
		super.onRestart();
	}

	@Override
	protected void onResume()
	{
		super.onResume();

		if ( mNeedWelcome )
		{
			AlertDialog.Builder welcome = new AlertDialog.Builder( SpeakerVerificationDemoActivity_Enroll.this );
			welcome.setMessage( "Please try pressing 'Enroll new user' button and read the text on screen to create your account!" )
			       .setPositiveButton( "OK", null );
			AlertDialog wel = welcome.create();
			wel.show();
		}
	}

	@Override
	protected void onPause()
	{
		if ( mIsEnrolling )
		{
			SpeakerVerificationJNI.Inst.stopenroll();
			mIsEnrolling   = false;
		}
		mNeedWelcome = false;

		super.onStop();
	}

	@Override
	protected void onDestroy()
	{
		SpeakerVerificationJNI.unregisterEventHandler( this, 0 );

		super.onDestroy();
	}

	public void NotifyCallback( int msgType, int ext1, int ext2 )
	{
		Log.i( TAG, "NotifyCallback" );
		mHandler.obtainMessage( msgType ).sendToTarget();
	}
}
