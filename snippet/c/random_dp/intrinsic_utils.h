#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include <stdint.h>
#include <sys/time.h>

/*
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
*/
typedef unsigned short bf16_t; // two bytes

static inline uint64_t get_cycles() {
    uint32_t high, low;
    asm volatile("rdtsc \n\t"
                 : "=a" (low),
                   "=d" (high));
    return ((uint64_t)high << 32) | (low);
} 

static inline double getmillisecs () {
    struct timeval tv;
    gettimeofday (&tv, nullptr);
    return tv.tv_sec * 1e3 + tv.tv_usec * 1e-3;
}

static inline void dump_register_m512(__m512i minput, int type) {
    union proxy {
        __m512i  vec;
        u_int8_t  u8[64];
        u_int16_t u16[32];
        u_int32_t u32[16];
    };

    proxy p;
    p.vec = minput;

    if (type == 8) {
        for (int i=0; i < 64; i++) {
            printf("[%d]=%d ", i, p.u8[i]);
        }
    } else if (type == 16) {
        for (int i=0; i < 32; i++) {
            printf("[%d]=%d ", i, p.u16[i]);
        }
    } else if (type == 32) {
        for (int i=0; i < 16; i++) {
            printf("[%d]=%d ", i, p.u32[i]);
        }
    } else if (type == 2) {
        for (int i=0; i < 64; i++) {
            u_int8_t c = p.u8[i];
            printf("\n c[%d]=0x%x : ", i, c);
            for (int j=0; j<8; j++) {
                printf("[%d]=%x ", i*8+j, (c >> j) & 0x1);
            }
        }
    }
}

static inline void dump_register_m256(__m256i minput, int type) {
    union proxy {
        __m256i  vec;
        u_int8_t  u8[32];
        u_int16_t u16[16];
        u_int32_t u32[8];
    };

    proxy p;
    p.vec = minput;

    if (type == 8) {
        for (int i=0; i < 32; i++) {
            printf("[%d]=%d ", i, p.u8[i]);
        }
    } else if(type == 16) {
        for (int i=0; i < 16; i++) {
            printf("[%d]=%d ", i, p.u16[i]);
        }
    } else if(type == 32) {
        for (int i=0; i < 8; i++) {
            printf("[%d]=%d ", i, p.u32[i]);
        }
    } else if(type == 2) {
        for (int i=0; i < 32; i++) {
            u_int8_t c = p.u8[i];
            printf("\n c[%d]=0x%x : ", i, c);
            for(int j=0; j<8; j++) {
                printf("[%d]=%x ", i*8+j, (c >> j) & 0x1);
            }
        }
    }
}

static inline void dump_register_m128f(__m128 minput) {
    union proxy {
        __m128 vec;
        float f32[4];
    };

    proxy p;
    p.vec = minput;

    for (int i=0; i < 4; i++) {
        printf("[%d]=%6.3f ", i, p.f32[i]);
    }
}

static inline void dump_register_m512f(__m512 minput) {
    union proxy {
        __m512 vec;
        float f32[8];
    };

    proxy p;
    p.vec = minput;

    for (int i=0; i < 8; i++) {
        printf("[%d]=%6.3f ", i, p.f32[i]);
    }
}

static inline void dump_register_mask(__mmask64 minput) {
    union proxy {
        __mmask64  vec;
        u_int8_t  u8[8];
    };

    proxy p;
    p.vec = minput;

    for (int i=0; i < 8; i++) {
        u_int8_t c = p.u8[i];
        printf("\n c[%d]=0x%x : ", i, c);
        for(int j=0; j<8; j++) {
            printf("[%d]=%x ", i*8+j, (c >> j) & 0x1);
        }
    }
}

static inline void dump_register_mask(__mmask32 minput) {
    union proxy {
        __mmask32  vec;
        u_int8_t  u8[4];
    };

    proxy p;
    p.vec = minput;

    for (int i=0; i < 4; i++) {
        u_int8_t c = p.u8[i];
        printf("\n c[%d]=0x%x : ", i, c);
        for(int j=0; j<8; j++) {
            printf("[%d]=%x ", i*8+j, (c >> j) & 0x1);
        }
    }
}
