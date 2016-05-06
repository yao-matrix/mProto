/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _DET_H )
#define _DET_H

#include "AudioDef.h"

AUD_Int32s det_init( void **phHandle, const char *pTemplate );
AUD_Int32s det_additem( void *hHandle, AUD_Int8s *candidateName, AUD_Double candidateScore );
AUD_Int32s det_free( void **phHandle );

#endif // _DET_H