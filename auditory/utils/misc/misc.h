/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#if !defined( _MISC_H_ )
#define _MISC_H_

// tick profile, granularity: us
long long time_gettime();

// random number generator
int    random_getseed();
void   random_setseed( int seed );
double random_getrand();

// MIPS estimation
#if defined( IPP )
unsigned long long mips_gettick();
#endif

#endif // _MISC_H_