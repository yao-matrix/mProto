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

extern AUD_Int32s test_pitch_tracking();
extern AUD_Int32s test_denoise();
extern AUD_Int32s test_vad();
extern AUD_Int32s test_vad_gmm();
extern AUD_Int32s test_preemphasis();
extern AUD_Int32s test_vq();
extern AUD_Int32s test_framecheck();
extern AUD_Int32s test_mfcc();
extern AUD_Int32s test_mfcc_individual();

TestEntry allTests[] = 
{
	{ "test pitch-tracking",                     test_pitch_tracking },
	{ "test denoise",                            test_denoise },
    { "test frame check",                        test_framecheck },
	{ "test vad",                                test_vad },
	{ "test vad_gmm",                            test_vad_gmm },
	{ "test preemphasis",                        test_preemphasis },
	{ "test VQ",                                 test_vq },
	{ "test mfcc",                               test_mfcc },
    { "test mfcc individual",                    test_mfcc_individual },
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
