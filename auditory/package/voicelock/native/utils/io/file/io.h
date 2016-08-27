#if !defined( _IO_H_ )
#define _IO_H_

#include "AudioDef.h"

/* file */
typedef struct
{
    AUD_Int32s sampleRate;
    AUD_Int32s bytesPerSample;
    AUD_Int32s channelNum;
    AUD_Int32s dataChunkBytes;
} AUD_Summary;

AUD_Int32s parseWavFromFile( AUD_Int8s *pFileName, AUD_Summary *pSummary );
AUD_Int32s readWavFromFile( AUD_Int8s *pFileName, AUD_Int16s *pBuf, AUD_Int32s bufLen );
AUD_Int32s writeWavToFile( AUD_Int8s *pFileName, AUD_Int16s *pBuf, AUD_Int32s sampleNum );

/* directory */
typedef struct _dir dir;

typedef struct _entry
{
    char name[256];
    int  type;
} entry;

#define FILE_TYPE_DIRECTORY 4
#define FILE_TYPE_FILE      5

dir*   openDir( const char *pPath );
entry* scanDir( dir *pDir );
void   closeDir( dir *pDir );
int    mkDir( char *pName, int mode );

#endif // _IO_H_
