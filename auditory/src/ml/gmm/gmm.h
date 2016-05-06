/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/
#ifndef _GMM_H_
#define _GMM_H_

#include "type/matrix.h"

#define MAX_GMMNAME_LENGTH 256

typedef struct
{
	AUD_Int8s  arGmmName[MAX_GMMNAME_LENGTH];
	AUD_Int32s numMix;                        // number of mixtures
	AUD_Int32s width;                         // actual mean, var, feature vector length

	AUD_Matrix means;
	AUD_Matrix cvars;
	AUD_Vector weights;
	AUD_Vector dets;

	AUD_Int32s sigIndex;

	AUD_Int32s step;                         // row step in mean, cvar & input feature array
} GmmModel;

#endif // _GMM_H_
