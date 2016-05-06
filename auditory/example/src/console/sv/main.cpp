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
#include <fcntl.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

typedef AUD_Int32s (*TestFunc)();

typedef struct _TestEntry 
{
	const char  *pName;
	TestFunc    fFunc;
} TestEntry;

extern AUD_Int32s sv_gmm();

TestEntry allTests[] = 
{
	{ "gmm based speaker verification", sv_gmm },
};

int main( int argc, char* argv[] )
{
	AUD_Int32s ret   = 0;
	AUD_Int32s data  = 0;
	AUD_Int32s i;

	AUD_Int32s caseNo  = -1;
	AUD_Int32s testNum = (AUD_Int32s)( sizeof(allTests) / sizeof(allTests[0]) );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );

	AUDLOG( "pls select desired test case with index:\n" );
	for ( i = 0; i < testNum; i++ )
	{
		AUDLOG( "\t%d - %s\n", i, allTests[i].pName );
	}
	data = scanf( "%d", &caseNo );
	AUD_ASSERT( caseNo >= 0 && caseNo < testNum );
	AUDLOG( "chosen test case is: %d\n", caseNo );

	ret = allTests[caseNo].fFunc();

	return ret;
}
