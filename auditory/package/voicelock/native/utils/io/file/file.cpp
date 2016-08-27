#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "AudioDef.h"
#include "AudioRecog.h"
#include "AudioUtil.h"

#include "io.h"

AUD_Int32s parseWavFromFile( AUD_Int8s *pFileName, AUD_Summary *pSummary )
{
    AUD_ASSERT( pFileName && pSummary );

    // naive skip non-wav file by suffix
    char suffix[256] = { 0, };
    for ( int i = 0; i < (int)strlen( (char*)pFileName ); i++ )
    {
        suffix[i] = tolower( pFileName[i] );
    }
    suffix[strlen( (char*)pFileName )] = '\0';
    if ( strstr( suffix, ".wav" ) == NULL )
    {
        return -2;
    }

    FILE       *fp = NULL;
    AUD_Int32s ret, data;
    fp = fopen( (const char*)pFileName, "rb" );
    if ( !fp )
    {
        // open file failed
        return -1;
    }

    // 1. check whether it's a valid wav file( RIFF WAVE Chunk )
    ret = fseek( fp, 8, SEEK_SET );
    AUD_ASSERT( ret == 0 );
    char riffFormat[5] = { 0, 0, 0, 0, 0 };
    data = fread( riffFormat, 1, 4, fp );
    if ( strcmp( riffFormat, "WAVE" ) )
    {
        // not a valid wave file
        fclose( fp );
        return -2;
    }

    // 2. get wav information( format Chunk )
      // channel nunmber
    pSummary->channelNum = 0;
    ret = fseek( fp, 10, SEEK_CUR );
    AUD_ASSERT( ret == 0 );
    data = fread( &(pSummary->channelNum), 2, 1, fp );

      // sample rate
    data = fread( &(pSummary->sampleRate), 4, 1, fp );

      // bytes per sample
    pSummary->bytesPerSample = 0;
    ret = fseek( fp, 4, SEEK_CUR );
    AUD_ASSERT( ret == 0 );
    data = fread( &(pSummary->bytesPerSample), 2, 1, fp );

    // 3. get data chunk( data Chunk )
    while ( 1 )
    {
        char dataID[3] = { 0, 0, 0 };
        data = fread( dataID, 1, 2, fp );
        if ( strcmp( (const char*)dataID, "da" ) )
        {
            continue;
        }
        else
        {
            data = fread( dataID, 1, 2, fp );
            if ( !strcmp( (const char*)dataID, "ta" ) )
            {
                data = fread( &(pSummary->dataChunkBytes), 4, 1, fp );
                break;
            }
            continue;
        }
    }

    fclose( fp );
    fp = NULL;

#if 0
    AUDLOG( "*******************\n" );
    AUDLOG( "\t channel number  : %d\n", pSummary->channelNum );
    AUDLOG( "\t sample rate     : %d\n", pSummary->sampleRate );
    AUDLOG( "\t bytes per sample: %d\n", pSummary->bytesPerSample );
    AUDLOG( "\t data size       : %d\n", pSummary->dataChunkBytes );
    AUDLOG( "*******************\n" );
#endif


    return 0;
}

// return stream length( greater than 0 ) if success, else return error code( less than 0 )
// only support 16bit-signed stream currently
AUD_Int32s readWavFromFile( AUD_Int8s *pFileName, AUD_Int16s *pBuf, AUD_Int32s bufLen )
{
    AUD_ASSERT( pFileName && pBuf && ( bufLen > 0 ) );

    AUD_Int32s ret, data;
    AUD_Int32s sampleNum;

    // naive skip non-wav file by suffix
    char suffix[256];
    for ( int i = 0; i < (int)strlen( (char*)pFileName ); i++ )
    {
        suffix[i] = tolower( pFileName[i] );
    }
    suffix[strlen( (char*)pFileName )] = '\0';
    if ( strstr( suffix, ".wav" ) == NULL )
    {
        return -2;
    }

    FILE *fp = NULL;
    fp = fopen( (const char*)pFileName, "rb" );
    if ( !fp )
    {
        // open file failed
        return -1;
    }

    // 1. check whether it's a valid wav file( RIFF WAVE Chunk )
    ret = fseek( fp, 8, SEEK_SET );
    AUD_ASSERT( ret == 0 );
    char riffFormat[5] = { 0, 0, 0, 0, 0 };
    data = fread( riffFormat, 1, 4, fp );
    if ( strcmp( riffFormat, "WAVE" ) )
    {
        // not a valid wave file
        fclose( fp );
        return -2;
    }

    // 2. read data chunk
    AUD_Int32s dataSize = 0;
    while ( 1 )
    {
        char dataID[3] = { 0, 0, 0 };
        data = fread( dataID, 1, 2, fp );
        if ( strcmp( (const char*)dataID, "da" ) )
        {
            continue;
        }
        else
        {
            data = fread( dataID, 1, 2, fp );
            if ( !strcmp( (const char*)dataID, "ta" ) )
            {
                data = fread( &dataSize, 4, 1, fp );
                break;
            }
            continue;
        }
    }

    //Max 128MB
    if ( dataSize <= 0 || dataSize > 1024*1024*256) {
        fclose( fp );
        return -3;
    }
    sampleNum = fread( pBuf, 2, dataSize / 2, fp );

    fclose( fp );
    fp = NULL;

    return sampleNum;
}

// write pcm stream to wav file
// only support 16bit-signed mono stream currently, take assumption on LITTLE-ENDIAN
// micro architecture
AUD_Int32s writeWavToFile( AUD_Int8s *pFileName, AUD_Int16s *pBuf, AUD_Int32s sampleNum )
{
    AUD_ASSERT( pFileName && pBuf && ( sampleNum > 0 ) );

    AUD_Int32s data;

    FILE *fp = NULL;
    fp = fopen( (const char*)pFileName, "wb" );
    if ( !fp )
    {
        // open file failed
        return -1;
    }

    // RIFF WAVE chunk
    fwrite( "RIFF", 1, 4, fp );

    int fileSize = 4 + 24 + ( 8 + sampleNum * 2 );
    data = fwrite( &fileSize, sizeof(int), 1, fp );

    data = fwrite( "WAVE", 1, 4, fp );

    // Format chunk
    data = fwrite( "fmt ", 1, 4, fp );

    int chunkSize = 16;
    data = fwrite( &chunkSize, sizeof(int), 1, fp );

    short format = 0x0001;
    data = fwrite( &format, sizeof(short), 1, fp );

    short channel = 1;
    data = fwrite( &channel, sizeof(short), 1, fp );

    int sampleRate = 16000;
    data = fwrite( &sampleRate, sizeof(int), 1, fp );

    int avgBytesPerSec = sampleRate * 2;
    data = fwrite( &avgBytesPerSec, sizeof(int), 1, fp );

    short blockAlign = 2;
    data = fwrite( &blockAlign, sizeof(short), 1, fp );

    short bitsPerSample = 8 * 2;
    data = fwrite( &bitsPerSample, sizeof(short), 1, fp );

    // data chunk
    data = fwrite( "data", 1, 4, fp );

    int dataSize = sampleNum * 2;
    data = fwrite( &dataSize, sizeof(int), 1, fp );

    data = fwrite( pBuf, sizeof(short), sampleNum, fp );

    fflush( fp );

    fclose( fp );
    fp = NULL;

    return 0;
}
