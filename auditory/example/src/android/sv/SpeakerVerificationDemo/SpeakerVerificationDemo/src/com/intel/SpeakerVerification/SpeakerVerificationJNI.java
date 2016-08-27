package com.intel.SpeakerVerification;

import android.util.Log;


public class SpeakerVerificationJNI
{
private static ISpeakerVerification[] mHandler;

private static native int sv_init();
private static native int sv_startlisten( String userName );
private static native int sv_stoplisten();
private static native int sv_startenroll( String userName );
private static native int sv_stopenroll();

private final static String TAG = "SpeakerVerificationJNI_JAVA";

public static SpeakerVerificationJNI Inst;

static
{
	Log.i( TAG, "System.loadLibrary();" );
	System.loadLibrary( "speakerverificationjni" );
	mHandler = new ISpeakerVerification[3];
	for ( int i = 0; i < 3; i++ )
	{
		mHandler[i] = null;
	}
}

public static void registerEventHandler( ISpeakerVerification h, int i )
{
	Log.i( TAG, "registerEventHandler" );

	mHandler[i] = h;
}


public static void unregisterEventHandler( ISpeakerVerification h, int i )
{
	Log.i( TAG, "registerEventHandler" );

	mHandler[i] = null;
}

public void NotifyCallback( int msgType, int ext1, int ext2 )
{
	Log.i( TAG, "WakeOnKeywordCallback" );
	for ( int i = 0; i < 3; i++ )
	{
		if ( mHandler[i] != null )
		{
			mHandler[i].NotifyCallback( msgType, ext1, ext2 );
		}
	}
}

public int initsv()
{
	return sv_init();
}

public int startlisten( String userName )
{
	return sv_startlisten( userName );
}

public int stoplisten()
{
	return sv_stoplisten();
}

public int startenroll( String userName )
{
	return sv_startenroll( userName );
}

public int stopenroll()
{
	return sv_stopenroll();
}

}

