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
 * Utility.java
 */

package com.intel.OOBWakeOnVoice;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Calendar;
import java.util.TimeZone;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class Utility
{
	private static String TAG = "OOBWOV_Utility";
	public static void tarAndUploadDir( String path, String prefix ) throws Exception
	{
		final int COMPRESS_BUFFER_LEN = 2048;
		File      folder              = new File( path );
		String[]  filename            = folder.list();

		if ( filename.length == 0 )
		{
			Log.e( TAG, "++++++++++++++++++++there is no file in this directory++++++++++++++++++++" );
			return;
		}

		BufferedInputStream origin = null;
		File zipDataFile           = new File( "/logs/stats/", prefix + "_data" );
		zipDataFile.delete();
		FileOutputStream fout      = null;
		try
		{
			fout = new FileOutputStream( zipDataFile );
		}
		catch ( FileNotFoundException e )
		{
			e.printStackTrace();
		}

		ZipOutputStream out    = new ZipOutputStream( new BufferedOutputStream( fout ) );
		byte            buf[]  = new byte[COMPRESS_BUFFER_LEN];
		try
		{
			for ( String pressingFiles:filename )
			{
				Log.d( TAG, "compressing: " + pressingFiles );
				FileInputStream fileInputStream;
				fileInputStream   = new FileInputStream( path + pressingFiles );
				origin            = new BufferedInputStream( fileInputStream, COMPRESS_BUFFER_LEN );
				ZipEntry zipEntry = new ZipEntry( pressingFiles );
				out.putNextEntry( zipEntry );
				int count;
				while ( ( count = origin.read( buf, 0, COMPRESS_BUFFER_LEN ) ) != -1 )
				{
					out.write( buf, 0, count );
				}
				origin.close();
				File defile = new File ( path + pressingFiles );
				defile.delete();
			}
		}
		catch ( IOException e )
		{
			e.printStackTrace();
		}
		finally
		{
			try
			{
				out.flush();
				out.close();
			}
			catch ( IOException e )
			{
				e.printStackTrace();
			}
		}

		// create trigger file
		Log.d( TAG, "creating trigger file" );
		File triggerFile = new File( "/logs/stats/" + prefix + "_trigger" );
		FileOutputStream ftrigger = null;
		try
		{
			ftrigger = new FileOutputStream( triggerFile );
			String triggerContent = "Type=" + prefix +"\nDebug";
			ftrigger.write( triggerContent.getBytes() );
			ftrigger.flush();
		}
		catch ( FileNotFoundException e )
		{
			e.printStackTrace();
		}
		catch ( IOException e )
		{
			e.printStackTrace();
		}
		finally
		{
			if ( ftrigger != null )
			{
				try
				{
					ftrigger.close();
				}
				catch ( IOException e )
				{
					e.printStackTrace();
				}
			}
		}

		Log.i( TAG, " ***************************** ^_^ I have zipped files ^_^ ****************************************" );
	}

	public static long setAlarmTime( int hour, int minute, int second )
	{
		Calendar currentTime = Calendar.getInstance( TimeZone.getTimeZone( "GMT" ) );
		Calendar targetTime  = Calendar.getInstance( TimeZone.getTimeZone( "GMT" ) );
		int      year        = currentTime.get( Calendar.YEAR );
		int      month       = currentTime.get( Calendar.MONTH );
		int      day         = currentTime.get( Calendar.DATE );
		targetTime.set( Calendar.YEAR, year );
		targetTime.set( Calendar.MONTH, month );
		targetTime.set( Calendar.DAY_OF_MONTH, day );
		targetTime.set( Calendar.HOUR_OF_DAY, hour );
		targetTime.set( Calendar.MINUTE, minute );
		targetTime.set( Calendar.SECOND, second );
		return targetTime.getTimeInMillis();
	}
}