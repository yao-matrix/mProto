/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _BUFRING_H_ )
#define _BUFRING_H_

#include "AudioDef.h"

typedef struct
{
    AUD_Int16s  *pData;
    AUD_Int32s  bufSize;  // in bytes
    AUD_Int32s  ringSize;
    AUD_Int32s  idx;
    AUD_Int32s  filledSize;
} AUD_BufRing;

AUD_Int32s createBufRing( AUD_BufRing *pBufRing );
AUD_Int32s writeToBufRing( AUD_BufRing *pBufRing, AUD_Int16s *pBuf );
AUD_Int32s readFromBufRing( AUD_BufRing *pBufRing, AUD_Int16s *pBuf );
AUD_Int32s clearBufRing( AUD_BufRing *pBufRing );
AUD_Int32s destroyBufRing( AUD_BufRing *pBufRing );

AUD_Int32s getFilledBufNum( AUD_BufRing *pBufRing );

#endif // _BUFRING_H_
