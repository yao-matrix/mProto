/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Huang, Johnathan( Intel Lab )
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#ifndef _DCT_H_
#define _DCT_H_

typedef struct
{
    int    startIdx;
    int    endIdx;
    int    len;
    int    Q;
    short  *w;
    int    *inBuf;
} Dct_16s;

void dct_init( Dct_16s **phDct, int startIdx, int endIdx, int len, int Q );
void dct_calc( Dct_16s *pDct, int *inBuf, int *outBuf );
void dct_free( Dct_16s **phDct );

#endif // _DCT_H_
