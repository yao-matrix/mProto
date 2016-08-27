#ifndef _SIGPROCESS_H_
#define _SIGPROCESS_H_

#include "AudioDef.h"

#if defined( PI )
#undef PI
#endif

#define PI  (3.14159265358979323846)
#define TPI (6.28318530717958647692) /* 2pi */

#define MAX_INT16S (32767)
#define MIN_INT16S (-32768)
#define MAX_INT16U (65535)
#define MAX_INT32S (2147483647L)
#define MIN_INT32S (-2147483648L)
#define MAX_INT32U (4294967295L)
#define MAX_INT64S (9223372036854775807LL) // 2 ^ 63 - 1

#define MIN_IEEE_NEG_DOUBLE (-1.8E+307)
#define MIN_IEEE_POS_DOUBLE (2.2E-308)

#define LOG_ZERO -1.0E10


// math utility
#define SATURATE_16s(a) ( (AUD_Int16s)( ((a) > MAX_INT16S) ? MAX_INT16S : ((a) < MIN_INT16S) ? MIN_INT16S : (a) ) )
#define SATURATE_32s(a) ( (AUD_Int32s)( ((a) > MAX_INT32S) ? MAX_INT32S : ((a) < MIN_INT32S) ? MIN_INT32S : (a) ) )

#define AUD_ISQR(x)   ( (AUD_Int64s)(x) *(x) )
#define MABS(x)       ( ( (x) >= 0 ) ? (x) : ( -(x) ) )
#define AUD_MIN(x, y) (((x) > (y)) ? (y) : (x))
#define AUD_MAX(x, y) (((x) > (y)) ? (x) : (y))

#define AUD_CIRPREV(x ,y) ( ((x) + (y) - 1) % (y) )
#define AUD_CIRNEXT(x ,y) ( ((x) + 1) % (y) )

AUD_Int32s clz( AUD_Int32s val );
AUD_Int32s fp_ln( AUD_Int32s val );
AUD_Int32s isqrt( AUD_Int32s n );
AUD_Double logadd( AUD_Double x, AUD_Double y );

// vector manipulation
AUD_Int32u variance( AUD_Int16s *x, AUD_Int32s len, AUD_Int32s shift );
AUD_Int32s mean( AUD_Int16s *x, AUD_Int32s len );

AUD_Int32s dotproduct( AUD_Int32s *x, AUD_Int32s *y, AUD_Int32s len, AUD_Int32s shift );

#endif // _SIGPROCESS_H_
