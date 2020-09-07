#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
#include <sys/time.h>
#include <inttypes.h>
#include <malloc.h> 
#include <string.h>
#include <math.h>


#define TIME_DIFF(a,b) ((b.tv_sec - a.tv_sec) * 1000000 + (b.tv_usec - a.tv_usec))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


/*
 * Algorithm 1: 3-order Tayor expansion
 *   from https://github.com/mozilla/LPCNet/blob/master/src/vec_avx.h
 *
 */
#ifdef __AVX2__

static float* exp8_approx(float* data, size_t num, float* results) {

    const __m256 log2_E = _mm256_set1_ps(1.44269504);
    const __m256 max_in = _mm256_set1_ps(50.f);
    const __m256 min_in = _mm256_set1_ps(-50.f);

    const __m256 K0 = _mm256_set1_ps(0.99992522f);
    const __m256 K1 = _mm256_set1_ps(0.69583354f);
    const __m256 K2 = _mm256_set1_ps(0.22606716f);
    const __m256 K3 = _mm256_set1_ps(0.078024523f);
    const __m256i mask = _mm256_set1_epi32(0x7fffffff);
   
    __m256  X, XF, Y;
    __m256i I;
 
    for (size_t i = 0; i < num; i += 8) {
        X = _mm256_loadu_ps(data + i);

        // pow(e, x) = pow(2, xlog2(e)) 
        //   i = floor(xlog2(e))
        //   f = xlog2(e) - floor(xlog2(e)) 
        // pow(e, x) = pow(2, i) * pow(2, f)
        X = _mm256_mul_ps(X, log2_E);
        X = _mm256_max_ps(min_in, _mm256_min_ps(max_in, X));
        XF = _mm256_floor_ps(X);
        // i
        I = _mm256_cvtps_epi32(XF);
        // f
        X = _mm256_sub_ps(X, XF);

        // pow(2, f) = K3*f^3 + K2*f^2 + K1*f + K0
        // K3 = 3*ln(2) - 2, K2 = 3 - 4*ln(2), K1 = ln(2), K0 = 1
        Y = _mm256_fmadd_ps(_mm256_fmadd_ps(_mm256_fmadd_ps(K3, X, K2), X, K1), X, K0);

        I = _mm256_slli_epi32(I, 23);
        Y = _mm256_castsi256_ps(_mm256_and_si256(mask, _mm256_add_epi32(I, _mm256_castps_si256(Y))));

        _mm256_store_ps(results + i, Y);
    }

   return results;
}


static float* exp8_faster_approx(float* data, size_t num, float* results) {

    const __m256 C1 = _mm256_set1_ps(1064872507.1541044f);
    const __m256 C2 = _mm256_set1_ps(12102203.161561485f);

    __m256  X, Y;
 
    for (size_t i = 0; i < num; i += 8) {
        X = _mm256_loadu_ps(data + i);

        Y = _mm256_castsi256_ps(_mm256_cvttps_epi32(_mm256_fmadd_ps(C2, X, C1)));

        _mm256_store_ps(results + i, Y);
    }

   return results;
}

#endif
#if defined(__AVX512F__)
static float* exp16_approx(float* data, size_t num, float* results) {

    const __m512 log2_E = _mm512_set1_ps(1.44269504);
    const __m512 max_in = _mm512_set1_ps(50.f);
    const __m512 min_in = _mm512_set1_ps(-50.f);

    const __m512 K0 = _mm512_set1_ps(0.99992522f);
    const __m512 K1 = _mm512_set1_ps(0.69583354f);
    const __m512 K2 = _mm512_set1_ps(0.22606716f);
    const __m512 K3 = _mm512_set1_ps(0.078024523f);
    const __m512i mask = _mm512_set1_epi32(0x7fffffff);
   
    __m512  X, XF, Y;
    __m512i I;
 
    for (size_t i = 0; i < num; i += 16) {
        X = _mm512_loadu_ps(data + i);

        // pow(e, x) = pow(2, xlog2(e)) 
        //   i = floor(xlog2(e))
        //   f = xlog2(e) - floor(xlog2(e)) 
        // pow(e, x) = pow(2, i) * pow(2, f)
        X = _mm512_mul_ps(X, log2_E);
        X = _mm512_max_ps(min_in, _mm512_min_ps(max_in, X));
        XF = _mm512_floor_ps(X);
        // i
        I = _mm512_cvtps_epi32(XF);
        // f
        X = _mm512_sub_ps(X, XF);

        // pow(2, f) = K3*f^3 + K2*f^2 + K1*f + K0
        // K3 = 3*ln(2) - 2, K2 = 3 - 4*ln(2), K1 = ln(2), K0 = 1
        Y = _mm512_fmadd_ps(_mm512_fmadd_ps(_mm512_fmadd_ps(K3, X, K2), X, K1), X, K0);

        I = _mm512_slli_epi32(I, 23);
        Y = _mm512_castsi512_ps(_mm512_and_si512(mask, _mm512_add_epi32(I, _mm512_castps_si512(Y))));

        _mm512_store_ps(results + i, Y);
    }

   return results;
}

static float* exp16_faster_approx(float* data, size_t num, float* results) {

    const __m512 C1 = _mm512_set1_ps(1064872507.1541044f);
    const __m512 C2 = _mm512_set1_ps(12102203.161561485f);

    __m512 X, Y;
 
    for (size_t i = 0; i < num; i += 16) {
        X = _mm512_loadu_ps(data + i);

        Y = _mm512_castsi512_ps(_mm512_cvttps_epi32(_mm512_fmadd_ps(C2, X, C1)));

        _mm512_store_ps(results + i, Y);
    }

   return results;
}

#endif

int main(int argc, char *argv[]) {
    size_t num_elms = 1024 * 1024 * 100;

    float *data   = NULL;
    float *results = NULL;

    size_t align_size = 64;

    struct timeval start, end;

    data = (float*)memalign(align_size, sizeof(float) * num_elms);
    results = (float*)memalign(align_size, sizeof(float) * num_elms);

    size_t loop = 1;

#if 1
    // fill buffers
    for (size_t i = 0; i < num_elms; i++) {
        data[i] = rand() / (float)RAND_MAX * 100.f - 50.f;
        results[i] = 0;
    }
#endif


#if 0
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < num_elms; i++) {
        results[i] = expf(data[i]);
    }
    gettimeofday(&end, NULL);

    printf("[Naive] value: e^(%f) = %f, time(ms): %f\n", data[1], results[1], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif



#if defined(__AVX2__)
#if 0
    gettimeofday(&start, NULL);
    exp8_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX2] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif
#if 0
    gettimeofday(&start, NULL);
    exp8_faster_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX2 Faster] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#endif

#if defined(__AVX512F__)
#if 1
    gettimeofday(&start, NULL);
    exp16_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX3] value: e^(%f) = %f, time(ms): %f\n", data[1], results[1], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 0
    gettimeofday(&start, NULL);
    exp16_faster_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX3 Faster] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif



#endif

    free(data);
    data = NULL;
    free(results);
    results = NULL;


    return 0;
}

