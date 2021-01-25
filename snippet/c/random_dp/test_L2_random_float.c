#include "intrinsic_utils.h"
#include <cassert>
#include <cstdio>

#define DIM 128

float fvec_L2sqr_ref(const float *x, const float *y, size_t d) {
    float sum = 0;
    for (size_t i = 0; i < d; i++) {
        float diff = x[i] - y[i];
        sum += diff * diff;
    }
    return sum;
}

float fvec_L2sqr_avx512(const float *x, const float *y, size_t d) {
    __m512 msum = _mm512_setzero_ps();

    while (d >= 16) {
        __m512 mx = _mm512_loadu_ps(x);
        x += 16;
        __m512 my = _mm512_loadu_ps(y);
        y += 16;
        __m512 mdiff = _mm512_sub_ps(mx, my);
        msum = _mm512_add_ps(msum, _mm512_mul_ps (mdiff, mdiff));
        d -= 16;
    }

    __m256 msum2 = _mm512_extractf32x8_ps(msum, 1);
    msum2 = _mm256_add_ps(msum2, _mm512_extractf32x8_ps(msum, 0));
    if (d >= 8) {
        __m256 mx = _mm256_loadu_ps(x);
        x += 8;
        __m256 my = _mm256_loadu_ps(y);
        y += 8;
        __m256 mdiff = _mm256_sub_ps(mx, my);
        msum2 = _mm256_add_ps(msum2, _mm256_mul_ps(mdiff, mdiff));
        d -= 8;
    }

    __m128 msum3 = _mm256_extractf128_ps(msum2, 1);
    msum3 = _mm_add_ps(msum3, _mm256_extractf128_ps(msum2, 0));
    if (d >= 4) {
        __m128 mx = _mm_loadu_ps(x);
        x += 4;
        __m128 my = _mm_loadu_ps(y);
        y += 4;
        __m128 mdiff = _mm_sub_ps(mx, my);
        msum3 = _mm_add_ps(msum3, _mm_mul_ps(mdiff, mdiff));
        d -= 4;
    }

    msum3 = _mm_hadd_ps(msum3, msum3);
    msum3 = _mm_hadd_ps(msum3, msum3);
    float sum = _mm_cvtss_f32(msum3);

    for (size_t i = 0; i < d; i++) {
        float diff = x[i] - (float)(y[i]);
        sum += diff * diff;
    }
    return sum;
}

void fvec_L2sqr_avx512_y3(const float *x, const float *y1,  const float *y2,const float *y3, size_t d, float *result1, float *result2, float *result3) {
    __m512 msum1_512 = _mm512_setzero_ps ();
    __m512 msum2_512 = _mm512_setzero_ps ();
    __m512 msum3_512 = _mm512_setzero_ps ();

    while (d >= 16) {
        __m512 mx = _mm512_loadu_ps(x);
        __m512 my1 = _mm512_loadu_ps(y1);
        __m512 my2 = _mm512_loadu_ps(y2);
        __m512 my3 = _mm512_loadu_ps(y3);
        
        __m512 mdiff1 = _mm512_sub_ps(mx, my1);
        __m512 mdiff2 = _mm512_sub_ps(mx, my2);
        __m512 mdiff3 = _mm512_sub_ps(mx, my3);
        msum1_512 = _mm512_add_ps(msum1_512, _mm512_mul_ps (mdiff1, mdiff1));
        msum2_512 = _mm512_add_ps(msum2_512, _mm512_mul_ps (mdiff2, mdiff2));
        msum3_512 = _mm512_add_ps(msum3_512, _mm512_mul_ps (mdiff3, mdiff3));

        d -= 16;
        x += 16;
        y1 += 16;
        y2 += 16;
        y3 += 16;
#if 0
        printf("\n dump msum1: \n");  dump_register_m512f(msum1_512);
        printf("\n dump msum2: \n");  dump_register_m512f(msum2_512);
        printf("\n dump msum3: \n");  dump_register_m512f(msum3_512);
        printf("\n dump d=%ld \n", d);
#endif
    }

    __m256 msum1_256 = _mm512_extractf32x8_ps(msum1_512, 1);
    __m256 msum2_256 = _mm512_extractf32x8_ps(msum2_512, 1);
    __m256 msum3_256 = _mm512_extractf32x8_ps(msum3_512, 1);
    msum1_256 = _mm256_add_ps(msum1_256, _mm512_extractf32x8_ps(msum1_512, 0));
    msum2_256 = _mm256_add_ps(msum2_256, _mm512_extractf32x8_ps(msum2_512, 0));
    msum3_256 = _mm256_add_ps(msum3_256, _mm512_extractf32x8_ps(msum3_512, 0));
    if (d >= 8) {
        __m256 mx = _mm256_loadu_ps(x);
        __m256 my1 = _mm256_loadu_ps(y1);
        __m256 my2 = _mm256_loadu_ps(y2);
        __m256 my3 = _mm256_loadu_ps(y3);
        __m256 mdiff1 = _mm256_sub_ps(mx, my1);
        __m256 mdiff2 = _mm256_sub_ps(mx, my2);
        __m256 mdiff3 = _mm256_sub_ps(mx, my3);
        msum1_256 = _mm256_add_ps(msum1_256, _mm256_mul_ps(mdiff1, mdiff1));
        msum2_256 = _mm256_add_ps(msum2_256, _mm256_mul_ps(mdiff2, mdiff2));
        msum3_256 = _mm256_add_ps(msum3_256, _mm256_mul_ps(mdiff3, mdiff3));
        x += 8;
        y1 += 8;
        y2 += 8;
        y3 += 8;
        d -= 8;
    }

    __m128 msum1_128 = _mm256_extractf128_ps(msum1_256, 1);
    __m128 msum2_128 = _mm256_extractf128_ps(msum2_256, 1);
    __m128 msum3_128 = _mm256_extractf128_ps(msum3_256, 1);
    msum1_128 = _mm_add_ps (msum1_128, _mm256_extractf128_ps(msum1_256, 0));
    msum2_128 = _mm_add_ps (msum2_128, _mm256_extractf128_ps(msum2_256, 0));
    msum3_128 = _mm_add_ps (msum3_128, _mm256_extractf128_ps(msum3_256, 0));
    if (d >= 4) {
        __m128 mx = _mm_loadu_ps(x);
        __m128 my1 = _mm_loadu_ps(y1);
        __m128 my2 = _mm_loadu_ps(y2);
        __m128 my3 = _mm_loadu_ps(y3);
        __m128 mdiff1 = _mm_sub_ps(mx, my1);
        __m128 mdiff2 = _mm_sub_ps(mx, my2);
        __m128 mdiff3 = _mm_sub_ps(mx, my3);
        msum1_128 = _mm_add_ps(msum1_128, _mm_mul_ps(mdiff1, mdiff1));
        msum2_128 = _mm_add_ps(msum2_128, _mm_mul_ps(mdiff2, mdiff2));
        msum3_128 = _mm_add_ps(msum3_128, _mm_mul_ps(mdiff3, mdiff3));
        x += 4;
        y1 += 4;
        y2 += 4;
        y3 += 4;
        d -= 4;
    }

    msum1_128 = _mm_hadd_ps(msum1_128, msum1_128);
    msum1_128 = _mm_hadd_ps(msum1_128, msum1_128);
    float sum1 = _mm_cvtss_f32(msum1_128);
    msum2_128 = _mm_hadd_ps(msum2_128, msum2_128);
    msum2_128 = _mm_hadd_ps(msum2_128, msum2_128);
    float sum2 = _mm_cvtss_f32(msum2_128);
    msum3_128 = _mm_hadd_ps(msum3_128, msum3_128);
    msum3_128 = _mm_hadd_ps(msum3_128, msum3_128);
    float sum3 = _mm_cvtss_f32(msum3_128);

    for (size_t i = 0; i < d; i++) {
        float diff1 = x[i] - y1[i];
        float diff2 = x[i] - y2[i];
        float diff3 = x[i] - y3[i];
        sum1 += diff1 * diff1;
        sum2 += diff2 * diff2;
        sum3 += diff3 * diff3;
    }
    *result1 = sum1;
    *result2 = sum2;
    *result3 = sum3;
}

float fvec_L2sqr_avx2(const float *x, const float *y, size_t d) {
    __m256 msum1 = _mm256_setzero_ps();

    while (d >= 8) {
        __m256 mx = _mm256_loadu_ps(x); x += 8;
        __m256 my = _mm256_loadu_ps(y); y += 8;
        const __m256 a_m_b1 = mx - my;
        msum1 += a_m_b1 * a_m_b1;
        d -= 8;
    }

    __m128 msum2 = _mm256_extractf128_ps(msum1, 1);
    msum2       += _mm256_extractf128_ps(msum1, 0);

    if (d >= 4) {
        __m128 mx = _mm_loadu_ps(x); x += 4;
        __m128 my = _mm_loadu_ps(y); y += 4;
        const __m128 a_m_b1 = mx - my;
        msum2 += a_m_b1 * a_m_b1;
        d -= 4;
    }
    msum2 = _mm_hadd_ps(msum2, msum2);
    msum2 = _mm_hadd_ps(msum2, msum2);
    float sum = _mm_cvtss_f32(msum2);

    for (size_t i = 0; i < d; i++) {
        float diff = x[i] - (float)(y[i]);
        sum += diff * diff;
    }
    return sum;
}

#define BATCH_FIRST 0
void test1() {
    size_t ny = 10000000;   // 10M
    int nsearch = 3000;
    int loop = 100;
    int d = DIM;

    float* x = new float[d];
    float* y = new float[d * ny];
    float* result = new float[nsearch];
    float* result2 = new float[nsearch];
    float* result3 = new float[nsearch];
    size_t offset[nsearch];
    
    memset(result, 0, nsearch*sizeof(float));
    memset(result2, 0, nsearch*sizeof(float));
    memset(result3, 0, nsearch*sizeof(float));
    
    for(int j = 0; j < d; j++) {
        x[j] = drand48() * 256;
    }

#if 0
    for(size_t i = 0; i < ny; i++) {
        for(int j = 0; j < d; j++) {
            y[d * i + j] = (drand48() * 256);
        }
    }
#else

    // randomly generate parts of data and copy them
    int ncircle = 100; // num of y that create data independantly
    for (int i = 0; i < ncircle; i++) {
        for (int j = 0; j < d; j++) {
            y[d * i + j] = (drand48() * 256);
        }
    }
    
    for (size_t g = 1; g < ny/ncircle; g++) {
        memcpy(&y[g * d * ncircle], y, d * ncircle * sizeof(float));
    }
#endif

    for (int i = 0; i < nsearch; i++) {
        offset[i] = (int)(drand48() * ny);
    }

    printf("\n run test1(), d=%d, ny=%ld, nsearch=%d, loop=%d, flag_batch_first=%d \n", d, ny, nsearch, loop, BATCH_FIRST);

    // ref implementation
    uint64_t t00 = get_cycles();
    int count1 = 0;
    while (count1 < loop) {
        for (int iy = 0; iy < nsearch; iy++) {
            result[iy] = fvec_L2sqr_ref (x, y + offset[iy] * d, d);
        }
        count1++;
    }
    uint64_t t1 = get_cycles() - t00;
    printf("\n------, t1 (ref) = %lu cycles \n", t1);
    
    // avx512 implementation
    t00 = get_cycles();
    count1 = 0;
    while (count1 < loop) {
        _mm_clflush(offset);
        _mm_clflush(y);
#if BATCH_FIRST == 1
        for (int iy = 0; iy < nsearch; iy += 3) {
            fvec_L2sqr_avx512_y3(x, y + offset[iy] * d, y + offset[iy + 1] * d, y + offset[iy + 2] * d, d, result3 + iy, result3 + iy + 1, result3 + iy + 2);
        }
#else
        for (int iy = 0; iy < nsearch; iy++) {
            result2[iy] = fvec_L2sqr_avx512 (x, y + offset[iy] * d, d);
        }
#endif
        count1++;
    }
    uint64_t t2 = get_cycles() - t00;
    printf("\n------, t2 (avx512) = %lu cycles, t2 / t1=%6.3f \n", t2, (float)t2 / t1);

    // avx512 new implementation
    t00 = get_cycles();
    count1 = 0;
    while (count1 < loop) {
        _mm_clflush(offset);
        _mm_clflush(y);
#if BATCH_FIRST == 0
        for (int iy = 0; iy < nsearch; iy += 3) {
            fvec_L2sqr_avx512_y3(x, y + offset[iy] * d, y + offset[iy + 1] * d, y + offset[iy + 2] * d, d, result3 + iy, result3 + iy + 1, result3 + iy + 2);
        }
#else
        for (int iy = 0; iy < nsearch; iy++) {
            result2[iy] = fvec_L2sqr_avx512 (x, y + offset[iy] * d, d);
        }
#endif
        count1++;
    }
    uint64_t t3 = get_cycles() - t00;
    printf("\n------, t3 (avx512) = %lu cycles, t3 / t1=%6.3f, t3 / t2=%6.3f \n", t3, (float)t3 / t1, (float)t3 / t2);
    
    // avx2 implementation
    count1 = 0;
    while (count1 < loop) {
        for (int iy = 0; iy < nsearch; iy++) {
            result2[iy] = fvec_L2sqr_avx2 (x, y + offset[iy] * d, d);
        }
        count1++;
    }
    uint64_t t4 = get_cycles() - t00;
    printf("\n------, t4 (avx2) = %lu cycles, t4 / t1=%6.3f \n", t4, (float)t4 / t1);
    
    // check result
#if 1
    int error_num = 0;
    for (int i = 0; i < nsearch; i++) {
        if (abs(result[i] - result2[i]) > 2) {
            if(error_num < 10) {
                printf("error: find mismatch: result[%d]=%8.6f, result2[%d]=%8.6f \n", i, result[i], i, result2[i]);
            }
            error_num++;
        }
#if 1
        if (abs(result[i] - result3[i]) > 2) {
            if (error_num < 10) {
                printf("error: find mismatch: result[%d]=%8.6f, result3[%d]=%8.6f \n", i, result[i], i, result3[i]);
            }
            error_num++;
        }
#endif
    }
    if (error_num == 0) {
        printf("Verified OK! \n");
    } else {
        printf("Verified error! error num=%d\n", error_num);
    }
#endif

    delete [] x;
    delete [] y;
    delete [] result;
    delete [] result2;
    delete [] result3;
}

int main() {
    test1();
}
