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

typedef struct
{
	AUD_Int8s  templateName[256];

	FILE       *positiveFp;
	FILE       *negativeFp;
} _AUD_Det;

AUD_Int32s det_init( void **phHandle, const char *pTemplate )
{
	_AUD_Det *pState = (_AUD_Det*)calloc( sizeof(_AUD_Det), 1 );
	AUD_ASSERT( pState );

	char *pToken = strrchr( (char*)pTemplate, (int)'.' );

	size_t len = 0;
	if ( pToken )
	{
		while ( pToken > (char*)pTemplate )
		{
			len++;
			pToken--;
		}
		AUD_ASSERT( len > 0 );
		len++;
	}
	else
	{
		len = strlen( (char*)pTemplate );
	}

	snprintf( (char*)pState->templateName, len, "%s", pTemplate );
	// AUDLOG( "template is: %s\n", (char*)pState->templateName );

	pToken = strrchr( (char*)pState->templateName, (int)'-' );
	if ( pToken )
	{
		pToken++;
	}
	else
	{
		pToken = (char*)pState->templateName;
	}

	char positiveFileName[256] = { 0, };
	snprintf( positiveFileName, 256, "./det/positive_%s.log", pToken );
	pState->positiveFp = fopen( positiveFileName, "ab+" );
	AUD_ASSERT( pState->positiveFp );

	char negativeFileName[256] = { 0, };
	snprintf( negativeFileName, 256, "./det/negative_%s.log", pToken );
	pState->negativeFp = fopen( negativeFileName, "ab+" );
	AUD_ASSERT( pState->negativeFp );

	AUDLOG( "\ncurrent template: %s, positive file: %s, negative file: %s\n", pState->templateName, positiveFileName, negativeFileName );

	*phHandle = pState;

	return 0;
}

AUD_Int32s det_additem( void *hHandle, AUD_Int8s *candidateName, AUD_Double candidateScore )
{
	_AUD_Det *pState = (_AUD_Det*)hHandle;

	size_t len = 0;
	len = strlen( (const char*)pState->templateName );

	AUD_Bool isMatch = !strncmp( (const char*)pState->templateName, (const char*)candidateName, len );

	if ( isMatch )
	{
		fprintf( pState->positiveFp, "%f\n", candidateScore );
	}
	else
	{
		fprintf( pState->negativeFp, "%f\n", candidateScore );
	}

	return 0;
}

AUD_Int32s det_free( void **phHandle )
{
	_AUD_Det *pState = (_AUD_Det*)(*phHandle);

	fclose( pState->positiveFp );
	fclose( pState->negativeFp );

	free( pState );

	*phHandle = NULL;

	return 0;
}
