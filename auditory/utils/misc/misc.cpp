/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/**********************************************************************
 * profiling, granularity: us
 **********************************************************************/
long long time_gettime()
{
	struct timespec t;

	t.tv_sec = t.tv_nsec = 0;
	clock_gettime( CLOCK_MONOTONIC, &t );

	return ( (long long)t.tv_sec * 1000000000LL + t.tv_nsec ) / 1000;
}


/**********************************************************************
 * generates an arbitary seed for the random number generator
 **********************************************************************/
int random_getseed()
{
	return ( (int)time( NULL ) );
}

/**********************************************************************
 * set the seed of the random number generator to a specific value
 **********************************************************************/
void random_setseed( int seed )
{
	srand( seed );

	return;
}

/**********************************************************************
 * return a double-precision pseudo random number in the interval [0,1)
 **********************************************************************/
double random_getrand()
{
	return (double)rand() / RAND_MAX;
}

#if defined( IPP )
/**********************************************************************
 * get MIPS in x86 platform by leverage IPP lib
 **********************************************************************/
#include <ippcore.h>
unsigned long long mips_gettick()
{
	return ippGetCpuClocks();
}
#endif