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
