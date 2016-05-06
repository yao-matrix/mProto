/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "AudioDef.h"
#include "AudioUtil.h"
#include "AudioRecog.h"
#include "AudioConfig.h"

typedef struct
{
	char       keywordName[256];
	AUD_Int32s numTP;
	AUD_Int32s numFP;
	AUD_Int32s numTN;
	AUD_Int32s numFN;
} KeywordStatistics;

typedef struct
{
	AUD_Int8s         currentKeyword[256];
	AUD_Double        currentThreshold;

#if DUMP_DETAIL
	FILE              *detailFp;
#endif

	FILE              *statisticsFp;
	FILE              *csvFp;

	KeywordStatistics *pKeywordStatistics;
	AUD_Int32s        keywordsNum;

	AUD_Int32s        numTP;
	AUD_Int32s        numFP;
	AUD_Int32s        numTN;
	AUD_Int32s        numFN;
} _AUD_BenchMark;

AUD_Int32s benchmark_init( void **phHandle, AUD_Int8s *methodName, const char* keywords[], AUD_Int32s keywordsNum )
{
	// AUDLOG( "key word num: %d\n", keywordsNum );
	_AUD_BenchMark *pState = (_AUD_BenchMark*)calloc( sizeof(_AUD_BenchMark), 1 );
	AUD_ASSERT( pState );

#if DUMP_DETAIL
	char detailFileName[256];
	snprintf( detailFileName, 256, "benchmark_%s_details.log", (char*)methodName );
	pState->detailFp = fopen( detailFileName, "ab+" );
	AUD_ASSERT( pState->detailFp );
#endif

	char statisticsFileName[256];
	snprintf( statisticsFileName, 256, "benchmark_%s_statistics.log", (char*)methodName );
	pState->statisticsFp = fopen( statisticsFileName, "ab+" );
	AUD_ASSERT( pState->statisticsFp );

	char csvFileName[256];
	snprintf( csvFileName, 256, "benchmark_%s_csv.log", (char*)methodName );
	pState->csvFp = fopen( csvFileName, "ab+" );
	AUD_ASSERT( pState->csvFp );

	pState->pKeywordStatistics = (KeywordStatistics*)calloc( sizeof(KeywordStatistics), keywordsNum );
	AUD_ASSERT( pState->pKeywordStatistics );
	for ( int i = 0; i < keywordsNum; i++ )
	{
		strcpy( pState->pKeywordStatistics[i].keywordName, keywords[i] );
	}
	pState->keywordsNum = keywordsNum;

	*phHandle = pState;

	return 0;
}

AUD_Int32s benchmark_newthreshold( void *hHandle, AUD_Double threshold )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)hHandle;

	for ( int i = 0; i < pState->keywordsNum; i++ )
	{
		pState->pKeywordStatistics[i].numTP = 0;
		pState->pKeywordStatistics[i].numFP = 0;
		pState->pKeywordStatistics[i].numTN = 0;
		pState->pKeywordStatistics[i].numFN = 0;
	}

	pState->currentThreshold = threshold;

#if DUMP_DETAIL
	fprintf( pState->detailFp, "***********threshold: %.2f ************\n", threshold );
#endif

	fprintf( pState->statisticsFp, "***********threshold: %.2f ************\n", threshold );

	return 0;
}

AUD_Int32s benchmark_newtemplate( void *hHandle, AUD_Int8s *templateName )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)hHandle;

	strcpy( (char*)pState->currentKeyword, (const char*)templateName );

	pState->numTP = 0;
	pState->numFP = 0;
	pState->numTN = 0;
	pState->numFN = 0;

#if DUMP_DETAIL
	fprintf( pState->detailFp, "\t***********keyword: %s ************\n", templateName );
#endif

	fprintf( pState->statisticsFp, "\t***********keyword: %s************\n", templateName );

	return 0;
}

AUD_Int32s benchmark_additem( void *hHandle, AUD_Int8s *candidateName, AUD_Double candidateScore, AUD_Bool isRecognized )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)hHandle;

	char   *pCommon = strrchr( (char*)pState->currentKeyword, (int)'-' );
	size_t len = 0;
	while ( pCommon > (char*)pState->currentKeyword )
	{
		len++;
		pCommon--;
	}

	AUD_Bool isMatch = !strncmp( (const char*)pState->currentKeyword, (const char*)candidateName, len );

	// keyword index
	AUD_Int32s keywordIndex = -1;
	for ( int i = 0; i < pState->keywordsNum; i++ )
	{
		if ( strstr( (const char*)pState->currentKeyword, (const char*)pState->pKeywordStatistics[i].keywordName ) )
		{
			keywordIndex = i;
			break;
		}
	}
	AUD_ASSERT( keywordIndex != -1 );

	char tag[3] = { 0, 0, 0 };

	if ( isMatch )
	{
		if ( isRecognized )
		{
			pState->numTP++;
			pState->pKeywordStatistics[keywordIndex].numTP++;
			strcpy( tag, "tp" );
		}
		else
		{
			pState->numFN++;
			pState->pKeywordStatistics[keywordIndex].numFN++;
			strcpy( tag, "fn" );
		}
	}
	else
	{
		if ( isRecognized )
		{
			pState->numFP++;
			pState->pKeywordStatistics[keywordIndex].numFP++;
			strcpy( tag, "fp" );
		}
		else
		{
			pState->numTN++;
			pState->pKeywordStatistics[keywordIndex].numTN++;
			strcpy( tag, "tn" );
		}
	}

#if DUMP_DETAIL
	if ( !strcmp( tag, "fp" ) || !strcmp( tag, "fn" ) )
	{
		fprintf( pState->detailFp, "%-30s  %-30s  %3.4f  %-3s\n", candidateName, pState->currentKeyword, candidateScore, tag );
	}
#endif

	return 0;
}

AUD_Int32s benchmark_finalizetemplate( void *hHandle )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)hHandle;

	fprintf( pState->statisticsFp, "\t total: %d  ", pState->numTP + pState->numTN + pState->numFP + pState->numFN );
	fprintf( pState->statisticsFp, "{ tp[%d], tn[%d], fp[%d], fn[%d] }\n", pState->numTP, pState->numTN, pState->numFP, pState->numFN );
	fprintf( pState->statisticsFp, "\t FAR(False Acceptance Rate): %.3f\n", pState->numFP / (AUD_Double)( pState->numTN + pState->numFP ) );
	fprintf( pState->statisticsFp, "\t FRR(False Rejection Rate) : %.3f\n", pState->numFN / (AUD_Double)( pState->numTP + pState->numFN ) );

	return 0;
}

AUD_Int32s benchmark_finalizethreshold( void *hHandle )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)hHandle;

	for ( int i = 0; i < pState->keywordsNum; i++ )
	{
		fprintf( pState->statisticsFp, "++++++++++++++++++++++++++++++++\n" );
		fprintf( pState->statisticsFp, "\tkeyword: %s\n", pState->pKeywordStatistics[i].keywordName );
		fprintf( pState->statisticsFp, "\t  total: %d  ", pState->pKeywordStatistics[i].numTP + pState->pKeywordStatistics[i].numTN + pState->pKeywordStatistics[i].numFP + pState->pKeywordStatistics[i].numFN );
		fprintf( pState->statisticsFp, " { tp[%d], tn[%d], fp[%d], fn[%d] }\n", pState->pKeywordStatistics[i].numTP, pState->pKeywordStatistics[i].numTN, pState->pKeywordStatistics[i].numFP, pState->pKeywordStatistics[i].numFN );
		fprintf( pState->statisticsFp, "\t  FAR(False Acceptance Rate): %.3f\n", pState->pKeywordStatistics[i].numFP / (AUD_Double)( pState->pKeywordStatistics[i].numFP + pState->pKeywordStatistics[i].numTN ) );
		fprintf( pState->statisticsFp, "\t  FRR(False Rejection Rate) : %.3f\n", pState->pKeywordStatistics[i].numFN / (AUD_Double)( pState->pKeywordStatistics[i].numTP + pState->pKeywordStatistics[i].numFN ) );
		fprintf( pState->statisticsFp, "++++++++++++++++++++++++++++++++\n" );

		fprintf( pState->csvFp, "%s, %.2f, %d, %d, %d, %d\n", pState->pKeywordStatistics[i].keywordName, pState->currentThreshold,\
		         pState->pKeywordStatistics[i].numTP, pState->pKeywordStatistics[i].numTN, pState->pKeywordStatistics[i].numFP, pState->pKeywordStatistics[i].numFN );
	}
	return 0;
}

AUD_Int32s benchmark_free( void **phHandle )
{
	_AUD_BenchMark *pState = (_AUD_BenchMark*)(*phHandle);

#if DUMP_DETAIL
	fclose( pState->detailFp );
#endif

	fclose( pState->statisticsFp );
	fclose( pState->csvFp );

	free( pState->pKeywordStatistics );
	free( pState );

	*phHandle = NULL;

	return 0;
}
