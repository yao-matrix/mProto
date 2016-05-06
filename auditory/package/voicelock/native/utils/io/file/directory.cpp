/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * NOTE: for pre-defined compiler Macros, you can refer to:
 *        http://sourceforge.net/p/predef/wiki/OperatingSystems/
 * Copyright(c) 2013 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if defined( _WIN32 )
#include <windows.h>
#include <FileAPI.h>

#elif defined( linux ) || defined( ANDROID )
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#include "io.h"

struct _dir
{
#if defined( _WIN32 )
    HANDLE           hDir;
    WIN32_FIND_DATA  hEntry;
    int              isFirstFile;
#elif defined( linux ) || defined( ANDROID )
    DIR*             hDir;
#endif

    _entry           file;
};

dir* openDir( const char *pPath )
{
    dir *pDir = (dir*)calloc( 1, sizeof(dir) );
    if ( pDir == NULL )
    {
        return NULL;
    }

#if defined( _WIN32 )
    char dirSpec[256] = { 0, };
    strncpy( dirSpec, pPath, strlen( pFilePath ) + 1 );
    strncat( dirSpec, "\*", 3 );

    pDir->hDir = FindFirstFile( dirSpec, &(pDir->hEntry) );
    if ( pDir->hDir == INVALID_HANDLE_VALUE )
    {
        printf( "Invalid file handle, Error is %u", GetLastError() );
        free( pDir );
        pDir = NULL;
    }
    else
    {
        pDir->isFirstFile = 1;
    }
#elif defined( linux ) || defined( ANDROID )
    pDir->hDir = opendir( pPath );
    if ( pDir->hDir == NULL )
    {
        printf( "Invalid file handle, Error is %d", errno );
        free( pDir );
        pDir = NULL;
    }
#else
    free( pDir );
    pDir = NULL;
    #error Not supported OS
    printf( "Not supported OS\n" );
#endif

    return pDir;
}

entry* scanDir( dir *pDir )
{
#if defined( _WIN32 )
    if ( isFirstFile == 0 )
    {
        int ret = FindNextFile( pDir->hDir, &(pDir->hEntry) );
        if ( ret == 0 )
        {
            return NULL;
        }
    }
    else
    {
        pDir->isFirstFile = 0;
    }

    strncpy( pDir->file.name, pDir->hEntry.cFileName, 255 );
    pDir->file.name[255] = '\0';

    if ( pDir->hEntry.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY )
    {
        pDir->file.type = FILE_TYPE_DIRECTORY;
    }
    else
    {
        pDir->file.type = FILE_TYPE_FILE;
    }

    return &(pDir->file);
#elif defined( linux ) || defined( ANDROID )
    struct dirent *pFile = readdir( pDir->hDir );
    if ( pFile == NULL )
    {
        return NULL;
    }

    strncpy( pDir->file.name, pFile->d_name, 255 );
    pDir->file.name[255] = '\0';

    if ( pFile->d_type == 4 )
    {
        pDir->file.type = FILE_TYPE_DIRECTORY;
    }
    else // 8
    {
        pDir->file.type = FILE_TYPE_FILE;
    }

    return &(pDir->file);
#else
    printf( "Not supported OS\n" );
    #error Not supported OS
    return NULL;
#endif
}

void closeDir( dir *pDir )
{
#if defined( _WIN32 )
    FindClose( pDir->hDir );
#elif defined( linux ) || defined( ANDROID )
    closedir( pDir->hDir );
#endif

    free( pDir );

    return;
}

int mkDir( char *pName, int mode )
{
    int ret;

#if defined( _WIN32 )
    ret = CreateDirectory( pName, NULL )
    if ( ret == 0 )
    {
        if ( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            ret = -1;
        }
        else
        {
            ret = -2;
        }
    }
    else
    {
        ret = 0;
    }
#elif defined( linux ) || defined( ANDROID )
    ret = mkdir( pName, mode );
    if ( ret == -1 )
    {
        if ( errno != EEXIST )
        {
            ret = -2;
        }
    }
#endif

    return ret;
}
