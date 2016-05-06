/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _AUD_UTIL_H )
#define _AUD_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define SWAPINT( x, y )  do { int t;  t = x; x = y; y = t; } while ( 0 )

// log mechanism
#define AUD_ERROR 3
#define AUD_WARN  2
#define AUD_INFO  1

#if defined( ANDROID )
#include <utils/Log.h>

#if defined( LOG_TAG )
#undef LOG_TAG
#define LOG_TAG "PSI-SpeechEngine"
#endif

#define TRACE( dbgLevel, ... )\
	do{\
		if ( dbgLevel == AUD_INFO )\
		{\
			ALOGV( __VA_ARGS__ );\
		}\
		else if ( dbgLevel == AUD_WARN )\
		{\
			ALOGW( __VA_ARGS__ );\
		}\
		else if ( dbgLevel == AUD_ERROR )\
		{\
			ALOGE( __VA_ARGS__ );\
		}\
	} while ( 0 )
#define AUDLOG ALOGW

#else
#if defined( AUD_LOG_INFO )
#define TRACE( dbgLevel, ... )\
do{\
	if ( dbgLevel >= AUD_INFO )\
	{\
		printf( __VA_ARGS__ );\
		fflush( NULL );\
	}\
} while ( 0 )

#elif defined( AUD_LOG_WARN )
#define TRACE( dbgLevel, ... )\
do{\
	if ( dbgLevel >= AUD_WARN )\
	{\
		printf( __VA_ARGS__ );\
		fflush( NULL );\
	}\
} while ( 0 )
#else
#define TRACE( dbgLevel, ... )\
	do {\
		if ( dbgLevel >= AUD_ERROR )\
		{\
			printf( __VA_ARGS__ );\
			fflush( NULL );\
		}\
} while ( 0 )
#endif

#define AUDLOG( ... ) do { printf( __VA_ARGS__ ); fflush( NULL ); } while ( 0 )

#endif // ANDROID

#define AUD_ASSERT( v )\
if ( !(v) )\
{\
	AUDLOG( "assertion failed: '%s' at File %s, Function %s, Line %d\n", #v, __FILE__, __FUNCTION__, __LINE__ );\
	exit( -1 );\
}

#define AUD_ECHO AUDLOG( "%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__ );

// function tag
#define AUD_FUNCTAG_E  AUDLOG( "Enter %s( %s, %d )\n", __FUNCTION__, __FILE__, __LINE__ );
#define AUD_FUNCTAG_X  AUDLOG( "Exit  %s( %s, %d )\n", __FUNCTION__, __FILE__, __LINE__ );

#endif // _AUD_UTIL_H
