/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <math.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "misc/misc.h"
#include "io/file/io.h"


int combine_gmm( )
{
	AUD_Int8s  inputFile1[256] = { 0, };
	AUD_Int8s  inputFile2[256] = { 0, };
	AUD_Int8s  modelName[256]  = { 0, };
	AUD_Int8s  modelPath[256]  = { 0, };
	AUD_Error  error           = AUD_ERROR_NONE;
	AUD_Tick   t;

	AUD_Int32s data;

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls input first model file:\n" );
	inputFile1[0] = '\0';
	data = scanf( "%s", inputFile1 );
	AUDLOG( "first model file is: %s\n", inputFile1 );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls input second model file:\n" );
	inputFile2[0] = '\0';
	data = scanf( "%s", inputFile2 );
	AUDLOG( "second model file is: %s\n", inputFile2 );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give generated GMM model name:\n" );
	data = scanf( "%s", modelName );
	AUDLOG( "generated GMM model name will be: %s\n", modelName );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );
	AUDLOG( "pls give the path where you want to store your generated GMM model:\n" );
	data = scanf( "%s", modelPath );

	AUDLOG( "generated GMM model file will be: %s/%s-combine.gmm\n", modelPath, modelName );

	void *hGmm1 = NULL;
	FILE *fpGmm1 = fopen( (const char*)inputFile1, "rb" );
	if ( fpGmm1 == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", inputFile1 );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hGmm1, fpGmm1 );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpGmm1 );
	fpGmm1 = NULL;

	void *hGmm2 = NULL;
	FILE *fpGmm2 = fopen( (const char*)inputFile2, "rb" );
	if ( fpGmm2 == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", inputFile2 );
		return AUD_ERROR_IOFAILED;
	}
	error = gmm_import( &hGmm2, fpGmm2 );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpGmm2 );
	fpGmm2 = NULL;

	// agglomerate GMM
	AUDLOG( "\nEM GMM combine Start\n" );
	t = time_gettime();
	void *hGmmHandle = NULL;
	error = gmm_mix( &hGmmHandle, hGmm1, hGmm2, modelName );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	t = time_gettime() - t;
	AUDLOG( "EM GMM combine Done\n" );

	gmm_show( hGmmHandle );

	// export GMM
	char modelFile[256] = { 0 };
	snprintf( modelFile, 256, "%s/%s-combine.gmm", modelPath, modelName );
	AUDLOG( "Export GMM Model File[%s] Start\n", modelFile );
	FILE *fpGmm = fopen( modelFile, "wb" );
	AUD_ASSERT( fpGmm );

	error = gmm_export( hGmmHandle, fpGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	AUDLOG( "Export GMM Model File Done\n" );

	fclose( fpGmm );
	fpGmm = NULL;

	error = gmm_free( &hGmmHandle );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	AUDLOG( "Finish EM GMM combination, Time Elapsed: %.2f s\n\n", t / 1000000.0 );

	return 0;
}
