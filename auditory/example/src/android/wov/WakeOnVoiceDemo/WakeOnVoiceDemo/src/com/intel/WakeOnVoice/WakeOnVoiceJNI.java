package com.intel.WakeOnVoice;

import android.util.Log;
import java.util.*;


public class WakeOnVoiceJNI
{
private static IWakeOnVoice[] mHandler;

public native int wov_init();
public native int wov_startlisten();
public native int wov_stoplisten();
public native int wov_startenroll();
public native int wov_enabledebug( int i );
public native int wov_setenrolltimes( int t );
public native int wov_setsecuritylevel( int s );

private final static String TAG = "WakeOnVoiceJNI_JAVA";

public static WakeOnVoiceJNI Inst;

static
{
	Log.i( TAG, "System.loadLibrary()" );
	System.loadLibrary( "wakeonvoicejni" );

	mHandler = new IWakeOnVoice[2];
}

public static void registerEventHandler( IWakeOnVoice h, int index )
{
	Log.i( TAG, "registerEventHandler" );

	mHandler[index] = h;
}

public void NotifyCallback( int msgType, int ext1, int ext2 )
{
	Log.i( TAG, "WakeOnKeywordCallback" );
	for ( int i = 0; i < 2; i++ )
	{
		mHandler[i].NotifyCallback( msgType, ext1, ext2 );
	}
}

public int initwov()
{
	return wov_init();
}

public int startlisten()
{
	return wov_startlisten();
}

public int stoplisten()
{
	return wov_stoplisten();
}

public int startenroll()
{
	return wov_startenroll();
}

public int enableDebug( int i )
{
	return wov_enabledebug( i );
}

public int setenrolltimes( int t )
{
	return wov_setenrolltimes( t );
}

public int setsecuritylevel( int s )
{
	return wov_setsecuritylevel( s );
}

}

