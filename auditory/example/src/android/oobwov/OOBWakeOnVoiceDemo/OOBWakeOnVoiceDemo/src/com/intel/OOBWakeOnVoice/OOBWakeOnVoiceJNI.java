package com.intel.OOBWakeOnVoice;

import android.util.Log;


public class OOBWakeOnVoiceJNI
{
private static IOOBWakeOnVoice[] mHandler;

public native int oobwov_init();
public native int oobwov_startlisten();
public native int oobwov_stoplisten();
public native int oobwov_startenroll();
public native int oobwov_enabledebug( int i );
public native int oobwov_setenrolltimes( int t );
public native int oobwov_setsecuritylevel( int s );

private final static String TAG = "OOBWakeOnVoiceJNI_JAVA";

public static OOBWakeOnVoiceJNI Inst;

static
{
	Log.i( TAG, "System.loadLibrary()" );
	System.loadLibrary( "oobwakeonvoicejni" );

	mHandler = new IOOBWakeOnVoice[2];
}

public static void registerEventHandler( IOOBWakeOnVoice h, int index )
{
	Log.i( TAG, "registerEventHandler" );

	mHandler[index] = h;
}

public void NotifyCallback( int msgType, int ext1, int ext2 )
{
	Log.i( TAG, "OOBWakeOnKeywordCallback" );
	for ( int i = 0; i < 2; i++ )
	{
		mHandler[i].NotifyCallback( msgType, ext1, ext2 );
	}
}

public int initwov()
{
	return oobwov_init();
}

public int startlisten()
{
	return oobwov_startlisten();
}

public int stoplisten()
{
	return oobwov_stoplisten();
}

public int startenroll()
{
	return oobwov_startenroll();
}

public int enableDebug( int i )
{
	return oobwov_enabledebug( i );
}

public int setenrolltimes( int t )
{
	return oobwov_setenrolltimes( t );
}

public int setsecuritylevel( int s )
{
	return oobwov_setsecuritylevel( s );
}

}

