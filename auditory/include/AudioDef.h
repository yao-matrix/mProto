/***************************************************************
 * Intel MCG PSI Core TR project: mAudio, where m means magic
 *   Author: Yao, Matrix( matrix.yao@intel.com )
 * Copyright(c) 2012 Intel Corporation
 * ALL RIGHTS RESERVED
 ***************************************************************/

#if !defined( _AUDIO_DEF_H )
#define _AUDIO_DEF_H

typedef unsigned char      AUD_Bool;
typedef unsigned char      AUD_Int8u;
typedef signed char        AUD_Int8s;
typedef unsigned short     AUD_Int16u;
typedef signed short       AUD_Int16s;
typedef unsigned int       AUD_Int32u;
typedef signed int         AUD_Int32s;
typedef unsigned long long AUD_Int64u;
typedef signed long long   AUD_Int64s;
typedef float              AUD_Float;
typedef double             AUD_Double;
typedef long long          AUD_Tick;

#define SIZEOF_TYPE( dataType )\
       ( ( dataType == AUD_DATATYPE_INT16S ) ?\
         sizeof(AUD_Int16s) :\
         ( ( dataType == AUD_DATATYPE_INT32S ) ?\
           sizeof(AUD_Int32s) :\
           ( ( dataType == AUD_DATATYPE_INT64S ) ?\
             sizeof(AUD_Int64s) :\
             ( ( dataType == AUD_DATATYPE_INT64S ) ?\
               sizeof(AUD_Float) :\
               ( ( dataType == AUD_DATATYPE_DOUBLE ) ?\
                 sizeof(AUD_Double) :\
                 -1\
               )\
              )\
            )\
          )\
        )

#define AUD_TRUE     ((AUD_Bool)(1))
#define AUD_FALSE    ((AUD_Bool)(0))

#define AUD_MAX_PATH 256

typedef enum
{
	AUD_ERROR_NONE        = 0,

	AUD_WARN_START        = 1000,
	AUD_WARN_REWIND       = 1001,
	
	AUD_ERROR_START       = 3000,
	AUD_ERROR_MOREDATA    = 3001,
	AUD_ERROR_OUTOFMEMORY = 3002,
	AUD_ERROR_IOFAILED    = 3003,
	AUD_ERROR_UNSUPPORT   = 3004,

	AUD_ERROR_LIMIT       = 0x7fffffff,
} AUD_Error;

typedef enum
{
	AUD_AMPCOMPRESS_NONE  = 0,
	AUD_AMPCOMPRESS_LOG   = 1,
	AUD_AMPCOMPRESS_ROOT  = 2,

	AUD_AMPCOMPRESS_LIMIT = 0x7fffffff,
} AUD_AmpCompress;

typedef enum
{
	AUD_TIMELINE_ROW   = 0,
	AUD_TIMELINE_COL   = 1,

	AUD_TIMELINE_LIMIT = 0x7fffffff,
} AUD_TimeLine;

typedef enum
{
	AUD_DATATYPE_INT8S    = 0,
	AUD_DATATYPE_INT16S   = 1,
	AUD_DATATYPE_INT32S   = 2,
	AUD_DATATYPE_INT64S   = 3,
	AUD_DATATYPE_FLOAT    = 4,
	AUD_DATATYPE_DOUBLE   = 5,

	AUD_DATATYPE_LIMIT    = 0x7fffffff,
	AUD_DATATYPE_INVALID  = AUD_DATATYPE_LIMIT,
} AUD_DataType;

#endif // _AUDIO_DEF_H
