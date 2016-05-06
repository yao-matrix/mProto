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
 * OOBWakeOnVoice_JNI.java
 *
 */

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

