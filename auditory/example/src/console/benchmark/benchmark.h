/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _BENCHMARK_H )
#define _BENCHMARK_H

#include "AudioDef.h"

AUD_Int32s benchmark_init( void **phHandle, AUD_Int8s *methodName, const char *keywords[], AUD_Int32s keywordsNum );
AUD_Int32s benchmark_newthreshold( void *hHandle, AUD_Double threshold );
AUD_Int32s benchmark_newtemplate( void *hHandle, AUD_Int8s *templateName );
AUD_Int32s benchmark_additem( void *hHandle, AUD_Int8s *candidateName, AUD_Double candidateScore, AUD_Bool isRecognized );
AUD_Int32s benchmark_finalizetemplate( void *hHandle );
AUD_Int32s benchmark_finalizethreshold( void *hHandle );
AUD_Int32s benchmark_free( void **phHandle );


#endif // _BENCHMARK_H