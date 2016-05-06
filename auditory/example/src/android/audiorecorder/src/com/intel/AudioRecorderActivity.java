package com.intel;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayDeque;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.text.format.Time;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Toast;

public class AudioRecorderActivity extends Activity {
	private final static String TAG = "AudioRecorderActivity";

	private Button      mStartBt; 
	private Button      mStopBt; 
	private int         mAudioSource       = MediaRecorder.AudioSource.MIC; 
	private static int  mSampleRateInHz    = 16000;
	private static int  mChannelConfig     = AudioFormat.CHANNEL_IN_MONO; 
	private static int  mAudioFormat       = AudioFormat.ENCODING_PCM_16BIT;
	private int         mBufferSizeInBytes = 0;
	private AudioRecord mAudioRecord       = null;; 
	private boolean     isRecord    = false;
	private String      mPcmName    = "";
	private String      mAudioName  = "";
	private String      mWavName    = "";
	private String      mStrKeyword = "";
	private String      mStrEnv     = "";
	private ProgressBar mProgressBar;
	private AlertDialog mAlertDialog;
	private byte[]      mAudioBuf;

	private ArrayDeque<Integer>  mEmptyBufRing = new ArrayDeque<Integer>( 2 );
	private ArrayDeque<Integer>  mFillBufRing  = new ArrayDeque<Integer>( 2 );

	private AlertDialog.Builder mKeywordBuilder;
	private AlertDialog.Builder mEnvBuilder;

	private Time     time  = new Time( "GMT+8" );


	@Override
	public void onCreate( Bundle savedInstanceState )
	{
		super.onCreate( savedInstanceState );
		setContentView( R.layout.main );

		mStartBt = (Button)this.findViewById( R.id.onButton ); 
		mStopBt  = (Button)this.findViewById( R.id.offButton ); 

		mProgressBar = (ProgressBar)this.findViewById( R.id.Recording );
		mProgressBar.setVisibility( 4 );

		mStartBt.setOnClickListener( new TestAudioListener() ); 
		mStopBt.setOnClickListener( new TestAudioListener() ); 
	}


	private void popDialog()
	{
		final CharSequence[] keyword = { "academy", "computer", "geniusbutton", "hamburger", 
		                                 "hello-blugenie", "hello-intel", "hey-vlingo",
		                                 "listen-to-me", "original", "start-listening", "wake-up", "noise" };

		final CharSequence[] environment = { "office", "car", "shuttle", "outside", "pub", "motion" };

		mKeywordBuilder = new AlertDialog.Builder( this );
		mKeywordBuilder.setTitle( "Pick a keyword" );
		mKeywordBuilder.setSingleChoiceItems( keyword, -1, new DialogInterface.OnClickListener()
		{
			public void onClick( DialogInterface dialog, int i )
			{
				mStrKeyword = keyword[i].toString();
				mAlertDialog.cancel();

				mAlertDialog = mEnvBuilder.create();
				mAlertDialog.setCancelable( false );
				mAlertDialog.show();
			}
		} );

		mEnvBuilder = new AlertDialog.Builder( this );
		mEnvBuilder.setTitle( "Pick an environment" );
		mEnvBuilder.setSingleChoiceItems( environment, -1, new DialogInterface.OnClickListener()
		{
			public void onClick( DialogInterface dialog, int i )
			{
				mStrEnv = environment[i].toString();
				mWavName = mAudioName + "-" + mStrKeyword + "-" + mStrEnv + ".wav";
				Toast.makeText( getApplicationContext(), mWavName, Toast.LENGTH_SHORT ).show();
				mAlertDialog.cancel();
				Log.i( TAG, mWavName );
				copyWaveFile( mPcmName, mWavName );
			}
		} );

		Runnable r = new Runnable()
		{
			public void run()
			{
				mAlertDialog = mKeywordBuilder.create();
				mAlertDialog.setCancelable( false );
				mAlertDialog.show();
			}
		};
		this.runOnUiThread( r );
	}

	class TestAudioListener implements OnClickListener
	{
		public void onClick( View v )
		{
			if ( v == mStartBt )
			{
				creatAudioRecord();
				startRecord();
				mProgressBar.setVisibility( 0 );
			}
			if ( v == mStopBt )
			{
				stopRecord(); 
				mProgressBar.setVisibility( 4 );
			}
		}
	}

	private void creatAudioRecord()
	{
		Log.i( TAG, "=========creatAudioRecord()===========" );
		mBufferSizeInBytes = AudioRecord.getMinBufferSize( mSampleRateInHz, mChannelConfig, mAudioFormat ) * 5;
		mAudioBuf = new byte[mBufferSizeInBytes * 2];
		mAudioRecord = new AudioRecord( mAudioSource, mSampleRateInHz, mChannelConfig, mAudioFormat, mBufferSizeInBytes );
	}

	private void startRecord()
	{
		// Log.i( TAG, "=========creatAudioRecord()===========1" );
		// long time = System.currentTimeMillis();
		// SimpleDateFormat formatter = new SimpleDateFormat( "yyyy-MM-dd hh:mm:ss" );
		// String date = formatter.format( new java.util.Date() );
		// Log.i( TAG, "=========creatAudioRecord()===========2" );
		time.setToNow();
		int year   = time.year;
		int month  = time.month + 1;
		int day    = time.monthDay;
		int hour   = time.hour;
		int minute = time.minute;

		Log.d( TAG, "!!!year: " + year + " month: " + month + " day: " + day + " hour: " + hour + " minute: " + minute );

		mAudioName = getString( R.string._sdcard_ ) + String.valueOf( year ) + String.valueOf( month ) + String.valueOf( day )
		             + String.valueOf( hour ) + String.valueOf( minute );
		mPcmName   = mAudioName + ".pcm";
		// Log.i( TAG, "=========creatAudioRecord()===========3" );
		mEmptyBufRing.add( 0 );
		mEmptyBufRing.add( 1 );
		mAudioRecord.startRecording();
		mStartBt.setClickable( false );
		isRecord = true;
		new Thread( new AudioRecordThread() ).start();
		new Thread( new AudioWriteThread() ).start();
	}

	private void stopRecord()
	{
		mStartBt.setClickable( true );
		close();
	}

	private void close()
	{
		if ( mAudioRecord != null )
		{
			System.out.println( "stopRecord" );
			isRecord = false;
			mAudioRecord.stop(); 
			mAudioRecord.release();
			mEmptyBufRing.clear();
			mFillBufRing.clear();
			mAudioRecord = null; 
		}
	}

	class AudioRecordThread implements Runnable
	{
		public void run()
		{
			readDataFromMic();
		}
	}

	class AudioWriteThread implements Runnable
	{
		public void run()
		{
			writeDataToFile();
			Log.i( TAG, mAudioName );
			popDialog();
		}
	}

	private void readDataFromMic()
	{
		int readSize = 0;

		while ( isRecord == true )
		{
			if ( mEmptyBufRing.isEmpty() )
			{
				// Log.d( TAG, "%%%%%%%%%%%%%%%%%%%%%" );
				try
				{
					Thread.sleep( 1 );
				} catch ( InterruptedException e )
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				continue;
			}

			int idx = mEmptyBufRing.remove();
			// Log.d( TAG, "read idx: " + idx );
			readSize = mAudioRecord.read( mAudioBuf, idx * mBufferSizeInBytes, mBufferSizeInBytes ); 
			if ( mAudioRecord.ERROR_INVALID_OPERATION != readSize )
			{
				// notify file write thread
				mFillBufRing.add( idx );
			}
		}
	}

	private void writeDataToFile()
	{
		FileOutputStream fos = null;

		try
		{
			File file = new File( mPcmName ); 
			if ( file.exists() )
			{
				file.delete();
			}
			fos = new FileOutputStream( file );
		}
		catch ( Exception e )
		{
			e.printStackTrace();
		}

		while ( isRecord == true )
		{
			if ( mFillBufRing.isEmpty() )
			{
				// Log.d( TAG, "aaaaaaaaaaaaa" );
				try {
					Thread.sleep( 1 );
				} catch ( InterruptedException e )
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				continue;
			}

			int idx = mFillBufRing.remove();
			// Log.d( TAG, "write idx: " + idx + ", mBufferSizeInBytes: " + mBufferSizeInBytes );
			try
			{
				fos.write( mAudioBuf, idx * mBufferSizeInBytes, mBufferSizeInBytes );
			} catch ( IOException e )
			{
				e.printStackTrace(); 
			}
			mEmptyBufRing.add( idx );
		}

		try
		{
			fos.close();
		} catch ( IOException e )
		{
			e.printStackTrace(); 
		}
	}

	private void copyWaveFile( String inFilename, String outFilename )
	{
		FileInputStream in   = null;
		FileOutputStream out = null;
		long totalAudioLen = 0;
		long totalDataLen = totalAudioLen + 36;
		long longSampleRate = mSampleRateInHz;
		int  channels = 1;
		long byteRate = 16 * mSampleRateInHz * channels / 8;
		byte[] data = new byte[mBufferSizeInBytes];
		try
		{
			in = new FileInputStream( inFilename );
			out = new FileOutputStream( outFilename );
			totalAudioLen = in.getChannel().size();
			totalDataLen = totalAudioLen + 36;
			WriteWaveFileHeader( out, totalAudioLen, totalDataLen, 
			                     longSampleRate, channels, byteRate );
			while ( in.read( data ) != -1 )
			{
				out.write( data );
			}
			in.close();
			out.close();
		}
		catch ( FileNotFoundException e )
		{
			e.printStackTrace();
		}
		catch ( IOException e )
		{
			e.printStackTrace();
		}
	}

	private void WriteWaveFileHeader( FileOutputStream out, long totalAudioLen, 
	                                  long totalDataLen, long longSampleRate, int channels, long byteRate ) 
	throws IOException
	{
		byte[] header = new byte[44];
		header[0] = 'R'; // RIFF/WAVE header
		header[1] = 'I';
		header[2] = 'F';
		header[3] = 'F';
		header[4] = (byte)(totalDataLen & 0xff);
		header[5] = (byte)((totalDataLen >> 8) & 0xff);
		header[6] = (byte)((totalDataLen >> 16) & 0xff);
		header[7] = (byte)((totalDataLen >> 24) & 0xff);
		header[8] = 'W';
		header[9] = 'A';
		header[10] = 'V';
		header[11] = 'E';
		header[12] = 'f'; // 'fmt' chunk
		header[13] = 'm';
		header[14] = 't';
		header[15] = ' ';
		header[16] = 16;  // 4 bytes: size of 'fmt' chunk
		header[17] = 0;
		header[18] = 0;
		header[19] = 0;
		header[20] = 1;   // format = 1
		header[21] = 0;
		header[22] = (byte)channels;
		header[23] = 0;
		header[24] = (byte)(longSampleRate & 0xff);
		header[25] = (byte)((longSampleRate >> 8) & 0xff);
		header[26] = (byte)((longSampleRate >> 16) & 0xff);
		header[27] = (byte)((longSampleRate >> 24) & 0xff);
		header[28] = (byte)(byteRate & 0xff);
		header[29] = (byte)((byteRate >> 8) & 0xff);
		header[30] = (byte)((byteRate >> 16) & 0xff);
		header[31] = (byte)((byteRate >> 24) & 0xff);
		header[32] = (byte)(2 * 8 / 8); // block align
		header[33] = 0;
		header[34] = 16; // bits per sample
		header[35] = 0;
		header[36] = 'd'; 
		header[37] = 'a'; 
		header[38] = 't';
		header[39] = 'a';
		header[40] = (byte)(totalAudioLen & 0xff);
		header[41] = (byte)((totalAudioLen >> 8) & 0xff);
		header[42] = (byte)((totalAudioLen >> 16) & 0xff);
		header[43] = (byte)((totalAudioLen >> 24) & 0xff);
		out.write( header, 0, 44 ); 
	}

	@Override 
	protected void onDestroy()
	{
		close(); 
		super.onDestroy(); 
	}
}

