#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include "AudioUtil.h"
#include "bufring.h"


// thread-unsafty buffer ring
AUD_Int32s createBufRing( AUD_BufRing *pBufRing )
{
    AUD_ASSERT( pBufRing );
    AUD_ASSERT( pBufRing->bufSize > 0 && pBufRing->ringSize > 0 );

    pBufRing->pData = NULL;
    pBufRing->pData = (AUD_Int16s*)calloc( pBufRing->bufSize * sizeof(AUD_Int16s) * pBufRing->ringSize, 1 );
    if ( pBufRing->pData == NULL )
    {
        return -1;
    }

    pBufRing->idx        = 0;
    pBufRing->filledSize = 0;

    return 0;
}

AUD_Int32s getFilledBufNum( AUD_BufRing *pBufRing )
{
    AUD_ASSERT( pBufRing );

    return pBufRing->filledSize;
}

AUD_Int32s writeToBufRing( AUD_BufRing *pBufRing, AUD_Int16s *pBuf )
{
    AUD_ASSERT( pBufRing && pBuf );

    memmove( pBufRing->pData + pBufRing->idx * pBufRing->bufSize, pBuf, pBufRing->bufSize * sizeof(AUD_Int16s) );

    pBufRing->idx = (pBufRing->idx + 1) % pBufRing->ringSize;

    if ( pBufRing->filledSize < pBufRing->ringSize )
    {
        (pBufRing->filledSize)++;
    }

    return 0;
}

AUD_Int32s readFromBufRing( AUD_BufRing *pBufRing, AUD_Int16s *pBuf )
{
    AUD_ASSERT( pBufRing && pBuf );

    if ( pBufRing->filledSize <= 0 )
    {
        return -1;
    }

    AUD_Int32s readPos = (pBufRing->idx + pBufRing->ringSize - pBufRing->filledSize) % pBufRing->ringSize;

    memmove( pBuf, pBufRing->pData + readPos * pBufRing->bufSize, pBufRing->bufSize * sizeof(AUD_Int16s) );

    (pBufRing->filledSize)--;

    return 0;
}

AUD_Int32s clearBufRing( AUD_BufRing *pBufRing )
{
    AUD_ASSERT( pBufRing );

    pBufRing->idx        = 0;
    pBufRing->filledSize = 0;

    return 0;
}

AUD_Int32s destroyBufRing( AUD_BufRing *pBufRing )
{
    AUD_ASSERT( pBufRing );

    if ( pBufRing->pData  )
    {
        free( pBufRing->pData );
        pBufRing->pData = NULL;
    }

    return 0;
}
