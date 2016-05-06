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

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"
#include "AudioConfig.h"

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"

#include "math/mmath.h"

// max allowable template number
#define TEMPLATE_MAX   5

AUD_Int32s test_dtw_tfi()
{
	AUD_Int8s  inputPath[256]    = { 0, };
	AUD_Int8s  templatePath[256] = { 0, };
	AUD_Double threshold         = 0.;
	AUD_Int32s data;
	AUD_Error error = AUD_ERROR_NONE;

	setbuf( stdin, NULL );
	AUDLOG( "pls give template wav stream's path:\n" );
	templatePath[0] = '\0';
	data = scanf( "%s", templatePath );
	AUDLOG( "template path is: %s\n", templatePath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give input wav stream's path:\n" );
	inputPath[0] = '\0';
	data = scanf( "%s", inputPath );
	AUDLOG( "input path is: %s\n", inputPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give score threshold:\n" );
	data = scanf( "%lf", &threshold );
	AUDLOG( "threshold is: %.2f\n", threshold );

	AUDLOG( "\n\n" );

	// file & directory operation, linux dependent
	entry              *pEntry  = NULL;
	dir                *pDir    = NULL;

	AUD_DTWSession     arDtwSession[TEMPLATE_MAX];
	AUD_Feature        arTemplate[TEMPLATE_MAX];
	AUD_Int32s         arTemplateSampleNum[TEMPLATE_MAX] = { 0, };
	AUD_Int32s         arTemplateWinNum[TEMPLATE_MAX]    = { 0, };
	AUD_Double         arScore[TEMPLATE_MAX]             = { 0., };
	AUD_Int8s          arTemplateStream[TEMPLATE_MAX][256];
	AUD_Int32s         templateNum = 0;

	AUD_Int32s         i, j;
	AUD_Int32s         sampleNum;
	AUD_Int32s         ret;

	AUD_Int32s         bufLen     = 0;
	AUD_Int16s         *pBuf   = NULL;


	// train template
	bufLen   = SAMPLE_RATE * BYTES_PER_SAMPLE * 10;
	pBuf     = (AUD_Int16s*)malloc( bufLen );
	AUD_ASSERT( pBuf );

	AUDLOG( "***********************************************\n" );
	AUDLOG( "template training start...\n" );

	templateNum = 0;
	pDir = openDir( (const char*)templatePath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		if ( templateNum >= TEMPLATE_MAX )
		{
			AUDLOG( "out of max supported template[%d], will skip the following files\n", TEMPLATE_MAX );
			break;
		}

		snprintf( (char*)(arTemplateStream[templateNum]), 256, "%s/%s", (const char*)templatePath, pEntry->name );

		ret = readWavFromFile( arTemplateStream[templateNum], pBuf, bufLen );
		if ( ret < 0 )
		{
			continue;
		}

		// request memeory for template
		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= arTemplateSampleNum[templateNum]; j++ )
		{
			;
		}

		AUDLOG( "pattern[%d: %s]: data segment size: %d, time slice number: %d\n", templateNum, arTemplateStream[templateNum], arTemplateSampleNum[templateNum], j );
	
		arTemplateWinNum[templateNum]                  = j;
		arTemplate[templateNum].featureMatrix.rows     = arTemplateWinNum[templateNum] - 2;
		arTemplate[templateNum].featureMatrix.cols     = FRAME_LEN - 2;
		arTemplate[templateNum].featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(arTemplate[templateNum].featureMatrix) );
		AUD_ASSERT( ret == 0 );
		
		arTemplate[templateNum].featureNorm.len      = arTemplateWinNum[templateNum] - 2;
		arTemplate[templateNum].featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(arTemplate[templateNum].featureNorm) );
		AUD_ASSERT( ret == 0 );

		void *hTfiHandle = NULL;

		// front end processing
		// pre-emphasis
		sig_preemphasis( pBuf, pBuf, arTemplateSampleNum[templateNum] );


		// init tfi handle
		error = tfi16s32s_init( &hTfiHandle, FRAME_LEN, WINDOW_TYPE, FRAME_STRIDE, AUD_AMPCOMPRESS_NONE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc tfi feature
		error = tfi16s32s_calc( hTfiHandle, pBuf, arTemplateSampleNum[templateNum], &(arTemplate[templateNum]) );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// deinit tfi handle
		error = tfi16s32s_deinit( &hTfiHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );
		
		templateNum++;
	}
	closeDir( pDir );
	free( pBuf );
	bufLen = 0;
	pBuf   = NULL;
	pDir   = NULL;
	pEntry = NULL;

	AUDLOG( "total template number: %d\n", templateNum );
	AUDLOG( "tempalte training finish...\n" );
	AUDLOG( "***********************************************\n\n\n" );

	// read data from input file/stream and do matching
	pDir = openDir( (const char*)inputPath );
	while ( ( pEntry = scanDir( pDir ) ) )
	{
		AUD_Int8s   inputStream[256] = { 0, };
		AUD_Summary fileSummary;
		void        *hTfiHandle      = NULL;

		snprintf( (char*)inputStream, 256, "%s/%s", (const char*)inputPath, pEntry->name );

		ret = parseWavFromFile( inputStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUDLOG( "input stream: %s\n", inputStream );

		bufLen = fileSummary.dataChunkBytes;
		pBuf   = (AUD_Int16s*)malloc( bufLen + 100 );
		AUD_ASSERT( pBuf );

		sampleNum = readWavFromFile( inputStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
		{
			;
		}

		AUD_Feature inFeature;
		inFeature.featureMatrix.rows     = j - 2;
		inFeature.featureMatrix.cols     = FRAME_LEN - 2;
		inFeature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		inFeature.featureNorm.len = j - 2;
		inFeature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );


		// init tfi handle
		error = tfi16s32s_init( &hTfiHandle, FRAME_LEN, WINDOW_TYPE, FRAME_STRIDE, AUD_AMPCOMPRESS_NONE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		error = tfi16s32s_calc( hTfiHandle, pBuf, sampleNum, &inFeature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// init dtw match
		for ( i = 0; i < templateNum; i++ )
		{
			arDtwSession[i].dtwType        = WOV_DTW_TYPE;
			arDtwSession[i].distType       = WOV_DISTANCE_METRIC;
			arDtwSession[i].transitionType = WOV_DTWTRANSITION_STYLE;
			error = dtw_initsession( &(arDtwSession[i]), &(arTemplate[i]), inFeature.featureMatrix.rows );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_updatefrmcost( &(arDtwSession[i]), &inFeature );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_match( &(arDtwSession[i]), WOV_DTWSCORING_METHOD, &(arScore[i]), NULL );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			error = dtw_deinitsession( &(arDtwSession[i]) );
			AUD_ASSERT( error == AUD_ERROR_NONE );

			if ( arScore[i] <= threshold )
			{
				AUDLOG( "\tFound pattern: %s, score: %.2f\n", arTemplateStream[i], arScore[i] );
			}
		}

		free( pBuf );
		pBuf   = NULL;
		bufLen = 0;

		ret = destroyMatrix( &(inFeature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(inFeature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		error = tfi16s32s_deinit( &hTfiHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );
	}
	closeDir( pDir );
	pDir = NULL;

	for ( i = 0; i < templateNum; i++ )
	{
		ret = destroyMatrix( &(arTemplate[i].featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(arTemplate[i].featureNorm) );
		AUD_ASSERT( ret == 0 );
	}

	AUDLOG( "DTW-TFI test finished\n" );

	return 0;
}
