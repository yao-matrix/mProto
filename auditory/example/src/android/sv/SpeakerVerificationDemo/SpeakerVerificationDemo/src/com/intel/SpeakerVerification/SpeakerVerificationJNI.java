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
 * Speaker_VerificationJNI.java
 *
 */

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

