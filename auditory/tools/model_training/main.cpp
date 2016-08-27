#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

typedef int (*TestFunc)();

typedef struct _TestEntry 
{
	const char  *pName;
	TestFunc    fFunc;
} TestEntry;

extern int train_gmm( );
extern int train_hmm();
extern int train_hmm_me();
extern int wov_adapt_gmm();
extern int sv_adapt_gmm();
extern int wov_adapt_gmm_si();
extern int train_hmm_si();
extern int kmeans_vq();
extern int combine_gmm();

TestEntry allTests[] = 
{
	{ "train gmm",              train_gmm },
	{ "combine gmm",            combine_gmm },
	{ "wov_adapt gmm",          wov_adapt_gmm },
	{ "wov_adapt gmm si",       wov_adapt_gmm_si },
	{ "sv_adapt gmm",           sv_adapt_gmm },
	{ "train hmm",              train_hmm },
	{ "train hmm multi-enroll", train_hmm_me },
	{ "train hmm si",           train_hmm_si },
	{ "generate vq codebook",   kmeans_vq },
};

int main( int argc, char* argv[] )
{
	AUD_Int32s ret  = 0;
	AUD_Int32s data = 0;
	AUD_Int32s i;

	AUD_Int32s caseNo  = -1;
	AUD_Int32s testNum = (AUD_Int32s)( sizeof(allTests) / sizeof(allTests[0]) );

	setbuf( stdout, NULL );
	setbuf( stdin, NULL );

	AUDLOG( "pls select desired training scheme with index:\n" );
	for ( i = 0; i < testNum; i++ )
	{
		AUDLOG( "\t%d - %s\n", i, allTests[i].pName );
	}
	data = scanf( "%d", &caseNo );
	AUD_ASSERT( caseNo >= 0 && caseNo < testNum );
	AUDLOG( "chosen training scheme is: %d\n", caseNo );

	ret = allTests[caseNo].fFunc();

	return ret;
}
