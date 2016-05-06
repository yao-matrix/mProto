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

extern AUD_Int32s test_dtw_tfi();
extern AUD_Int32s test_dtw_mfcc();
extern AUD_Int32s test_dtw_mfcc_cascade();
extern AUD_Int32s test_hmm_mfcc();
extern AUD_Int32s perf_dtw_mfcc();
extern AUD_Int32s perf_dtw_mfcc_vad();
extern AUD_Int32s perf_dtw_mfcc_frameadmit();
extern AUD_Int32s perf_dtw_mfcc_multienroll_naive();
extern AUD_Int32s perf_dtw_mfcc_multienroll_construct();
// extern AUD_Int32s perf_dtw_mfcc_multienroll_ss();
extern AUD_Int32s perf_hmm_mfcc();
extern AUD_Int32s perf_gmm_mfcc();

TestEntry allTests[] = 
{
	{ "test dtw-tfi",                            test_dtw_tfi },
	{ "test dtw-mfcc",                           test_dtw_mfcc },
	{ "test dtw-mfcc-cascade",                   test_dtw_mfcc_cascade },
	{ "test hmm-mfcc",                           test_hmm_mfcc },
	{ "perf dtw-mfcc",                           perf_dtw_mfcc },
	{ "perf dtw-mfcc-vad",                       perf_dtw_mfcc_vad },
	{ "perf dtw-mfcc-frameadmit",                perf_dtw_mfcc_frameadmit },
	{ "perf dtw-mfcc-naive-multi-enroll",        perf_dtw_mfcc_multienroll_naive },
	{ "perf dtw-mfcc-constructive-multi-enroll", perf_dtw_mfcc_multienroll_construct },
	// { "perf dtw-mfcc-ss-multi-enroll",           perf_dtw_mfcc_multienroll_ss },
	{ "perf hmm-mfcc",                           perf_hmm_mfcc },
	{ "perf gmm-mfcc",                           perf_gmm_mfcc },
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
