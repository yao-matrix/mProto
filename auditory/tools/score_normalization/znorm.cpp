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

#include "type/matrix.h"
#include "misc/misc.h"
#include "io/file/io.h"

#include "math/mmath.h"

/* target model agaist imposter utterance */

AUD_Int32s znorm()
{
	AUD_Error  error             = AUD_ERROR_NONE;
	AUD_Int8s  imposterPath[256] = { 0, };
	AUD_Int8s  znormGmmFile[256] = { 0, };
	AUD_Int8s  ubmGmmFile[256]   = { 0, };

	AUD_Int32s ret, data;
	AUD_Int32s i;

	setbuf( stdin, NULL );
	AUDLOG( "pls give imposter wav stream's path:\n" );
	imposterPath[0] = '\0';
	data = scanf( "%s", imposterPath );
	AUDLOG( "test stream path is: %s\n", imposterPath );

	setbuf( stdin, NULL );
	AUDLOG( "pls give gmm model file you want to do z-norm:\n" );
	znormGmmFile[0] = '\0';
	data = scanf( "%s", znormGmmFile );
	AUDLOG( "gmm model to do z-norm is: %s\n", znormGmmFile );

	setbuf( stdin, NULL );
	AUDLOG( "pls give ubm model file:\n" );
	ubmGmmFile[0] = '\0';
	data = scanf( "%s", ubmGmmFile );
	AUDLOG( "ubm model file is: %s\n", ubmGmmFile );

	void *hZnormGmm = NULL;
	FILE *fpZnormGmm = fopen( (const char*)znormGmmFile, "rb" );
	if ( fpZnormGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", znormGmmFile );
		return AUD_ERROR_IOFAILED;
	}

	AUDLOG( "import znorm gmm model from: [%s]\n", znormGmmFile );
	error = gmm_import( &hZnormGmm, fpZnormGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpZnormGmm );
	fpZnormGmm = NULL;

	// AUDLOG( "znorm gmm model is(%s, %s, %d):\n", __FILE__, __FUNCTION__, __LINE__ );
	// gmm_show( hZnormGmm );

	void *hUbmGmm = NULL;
	FILE *fpUbmGmm = fopen( (const char*)ubmGmmFile, "rb" );
	if ( fpUbmGmm == NULL )
	{
		AUDLOG( "cannot open gmm model file: [%s]\n", ubmGmmFile );
		return AUD_ERROR_IOFAILED;
	}

	AUDLOG( "import znorm gmm model from: [%s]\n", ubmGmmFile );
	error = gmm_import( &hUbmGmm, fpUbmGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );
	fclose( fpUbmGmm );
	fpUbmGmm = NULL;

	// count imposter wav number
	AUD_Int32s    imposterNum = 0;
	dir           *pImposterDir   = openDir( (const char*)imposterPath );
	entry         *pImposterEntry = NULL;

	while ( ( pImposterEntry = scanDir( pImposterDir ) ) )
	{
		char suffix[256] = { 0, };
		for ( i = 0; i < (AUD_Int32s)strlen( (char*)pImposterEntry->name ); i++ )
		{
			suffix[i] = tolower( pImposterEntry->name[i] );
		}

		if ( strstr( suffix, ".wav" ) == NULL )
		{
			continue;
		}

		imposterNum++;
	}
	closeDir( pImposterDir );
	pImposterDir = NULL;

	AUDLOG( "totally %d wave files\n", imposterNum );
	AUD_ASSERT( imposterNum > 0 );

	AUD_Double *pImposterScores = (AUD_Double*)calloc( imposterNum * sizeof(AUD_Double), 1 );
	AUD_ASSERT( pImposterScores );


	AUD_Int32s    idx = 0;
	pImposterDir   = openDir( (const char*)imposterPath );
	pImposterEntry = NULL;
	while ( ( pImposterEntry = scanDir( pImposterDir ) ) )
	{
		AUD_Int8s   imposterStream[256] = { 0, };
		void        *hMfccHandle        = NULL;
		AUD_Summary fileSummary;

		snprintf( (char*)imposterStream, 256, "%s/%s", (const char*)imposterPath, pImposterEntry->name );

		ret = parseWavFromFile( imposterStream, &fileSummary );
		if ( ret < 0 )
		{
			continue;
		}
		AUD_ASSERT( fileSummary.channelNum == CHANNEL_NUM && fileSummary.bytesPerSample == BYTES_PER_SAMPLE && fileSummary.sampleRate == SAMPLE_RATE );
		AUDLOG( "input stream: %s, keyword: %s\n", imposterStream, pImposterEntry->name );

		AUD_Int32s bufLen = fileSummary.dataChunkBytes;
		AUD_Int16s *pBuf  = (AUD_Int16s*)calloc( bufLen + 100, 1 );
		AUD_ASSERT( pBuf );

		AUD_Int32s sampleNum = readWavFromFile( imposterStream, pBuf, bufLen );
		AUD_ASSERT( sampleNum > 0 );

		AUD_Int32s j = 0;
		// segment number of the stream
		for ( j = 0; j * FRAME_STRIDE + FRAME_LEN <= sampleNum; j++ )
		{
			;
		}

		AUD_Feature feature;
		feature.featureMatrix.rows     = j - MFCC_DELAY;
		feature.featureMatrix.cols     = MFCC_FEATDIM;
		feature.featureMatrix.dataType = AUD_DATATYPE_INT32S;
		ret = createMatrix( &(feature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		feature.featureNorm.len      = j - MFCC_DELAY;
		feature.featureNorm.dataType = AUD_DATATYPE_INT64S;
		ret = createVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		// front end processing
		 // pre-emphasis
		sig_preemphasis( pBuf, pBuf, sampleNum );


		// init mfcc handle
		error = mfcc16s32s_init( &hMfccHandle, FRAME_LEN, WINDOW_TYPE, MFCC_ORDER, FRAME_STRIDE, SAMPLE_RATE, COMPRESS_TYPE );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc mfcc feature
		error = mfcc16s32s_calc( hMfccHandle, pBuf, sampleNum, &feature );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		// calc gmm score
		AUD_Vector compLlr;
		compLlr.len      = gmm_getmixnum( hZnormGmm );
		compLlr.dataType = AUD_DATATYPE_DOUBLE;
		ret = createVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		AUD_Vector bestIndex;
		bestIndex.len      = 10;
		bestIndex.dataType = AUD_DATATYPE_INT32S;
		ret = createVector( &bestIndex );
		AUD_ASSERT( ret == 0 );

		AUD_Double speakerScore = 0., ubmScore = 0.;
		AUD_Double frameScore   = 0., finalScore = 0.;

		for ( i = 0; i < feature.featureMatrix.rows; i++ )
		{
			frameScore = gmm_llr( hZnormGmm, &(feature.featureMatrix), i, &compLlr );

			// get top N gaussian component
			// showVector( &compLlr );
			ret = sortVector( &compLlr, &bestIndex );
			AUD_ASSERT( ret == 0 );
			// showVector( &bestIndex );

			frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
			for( j = 1; j < bestIndex.len; j++ )
			{
				frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
			}
			speakerScore += frameScore;

			frameScore = gmm_llr( hUbmGmm, &(feature.featureMatrix), i, &compLlr );

			frameScore = compLlr.pDouble[bestIndex.pInt32s[0]];
			for( j = 1; j < bestIndex.len; j++ )
			{
				frameScore = logadd( frameScore, compLlr.pDouble[bestIndex.pInt32s[j]] );
			}

			ubmScore += frameScore;
		}

		speakerScore /= feature.featureMatrix.rows;
		ubmScore     /= feature.featureMatrix.rows;

		finalScore = speakerScore - ubmScore;
		AUDLOG( "\tspeakerscore: %.2f, ubm score: %.2f, final score: %.2f\n", speakerScore, ubmScore, finalScore );

		// write z-norm score into array
		pImposterScores[idx] = finalScore;
		idx++;

		// deinit mfcc handle
		error = mfcc16s32s_deinit( &hMfccHandle );
		AUD_ASSERT( error == AUD_ERROR_NONE );

		ret = destroyMatrix( &(feature.featureMatrix) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &(feature.featureNorm) );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &bestIndex );
		AUD_ASSERT( ret == 0 );

		ret = destroyVector( &compLlr );
		AUD_ASSERT( ret == 0 );

		free( pBuf );
		pBuf   = NULL;
		bufLen = 0;
	}
	closeDir( pImposterDir );
	pImposterDir = NULL;

	// calc imposter score mean & std deviance
	AUD_Double mean = 0., std = 0.;
	for ( i = 0; i < imposterNum; i++ )
	{
		mean += pImposterScores[i];
	}
	mean /= imposterNum;

	for ( i = 0; i < imposterNum; i++ )
	{
		std += ( pImposterScores[i] - mean ) * ( pImposterScores[i] - mean );
	}
	std /= imposterNum;
	std = sqrt( std );

	// write to znorm file
	FILE *fpZnorm= fopen( "./znorm.txt", "ab+" );
	if ( fpZnorm == NULL )
	{
		AUDLOG( "cannot open ./znorm.txt\n" );
		return AUD_ERROR_IOFAILED;
	}
	fprintf( fpZnorm, "%s: %f, %f\n", znormGmmFile, mean, std );

	fclose( fpZnorm );
	fpZnorm = NULL;

	error = gmm_free( &hZnormGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	error = gmm_free( &hUbmGmm );
	AUD_ASSERT( error == AUD_ERROR_NONE );

	free( pImposterScores );
	pImposterScores = NULL;

	AUDLOG( "GMM Z-Norm finished\n" );

	return 0;
}
