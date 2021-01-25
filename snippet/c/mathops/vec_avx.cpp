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


#ifdef __AVX2__
/*
 * Algorithm 1: integer expansion
 *   from https://gist.github.com/andersx/8057b2a6fd3d715d35eb
 *
 */
static float* exp8_fastest_approx(float* data, size_t num, float* results) {

    const __m256 C0 = _mm256_set1_ps(1064872507.1541044f);
    const __m256 C1 = _mm256_set1_ps(12102203.161561485f);

    __m256  X, Y;
 
    for (size_t i = 0; i < num; i += 8) {
        X = _mm256_loadu_ps(data + i);

        Y = _mm256_castsi256_ps(_mm256_cvttps_epi32(_mm256_fmadd_ps(C1, X, C0)));

        _mm256_store_ps(results + i, Y);
    }

   return results;
}

/*
 * Algorithm 2: 3-order Tayor expansion
 *   from https://github.com/mozilla/LPCNet/blob/master/src/vec_avx.h
 * Original from celt:
     https://chromium.googlesource.com/chromium/deps/opus/+/1.0.x/celt/mathops.h
 *
 */
static float* exp8_faster_approx(float* data, size_t num, float* results) {

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


/*
 * Algorithm 3: 7-order Tayor expansion
 *   from https://github.com/PaddlePaddle/Paddle/blob/develop/paddle/fluid/operators/math/detail/avx_mathfun.h
 * Original algorithm
 *   from http://gruntthepeon.free.fr/ssemath/sse_math.h
 * This one is also used by eigen (https://gitlab.com/libeigen/-/blob/master/Eigen/src/Core/arch/AVX512/MathFunctions.h)
 */
static float* exp8_fast_approx(float* data, size_t num, float* results) {

    __m256  x, fx, tmp, cmask, z, y, pow2n;
    __m256i imm0;

    const __m256 one = _mm256_set1_ps(1.f);
    const __m256 p5 = _mm256_set1_ps(0.5);
    const __m256 max_in = _mm256_set1_ps(88.3762626647949f);
    const __m256 min_in = _mm256_set1_ps(-88.3762626647949f);

    const __m256 cephes_LOG2EF = _mm256_set1_ps(1.44269504088896341);
    const __m256 cephes_exp_C1 = _mm256_set1_ps(0.693359375);
    const __m256 cephes_exp_C2 = _mm256_set1_ps(-2.12194440e-4);

    const __m256 cephes_exp_p0 = _mm256_set1_ps(1.9875691500E-4);
    const __m256 cephes_exp_p1 = _mm256_set1_ps(1.3981999507E-3);
    const __m256 cephes_exp_p2 = _mm256_set1_ps(8.3334519073E-3);
    const __m256 cephes_exp_p3 = _mm256_set1_ps(4.1665795894E-2);
    const __m256 cephes_exp_p4 = _mm256_set1_ps(1.6666665459E-1);
    const __m256 cephes_exp_p5 = _mm256_set1_ps(5.0000001201E-1);
    const __m256i offset = _mm256_set1_epi32(0x7f);
    const __m256i mask = _mm256_set1_epi32(0x7fffffff);


    for (size_t i = 0; i < num; i += 8) {
        x = _mm256_loadu_ps(data + i);

        x = _mm256_min_ps(x, max_in);
        x = _mm256_max_ps(x, min_in);

        /* express exp(x) as exp(g + n*log(2)) */
        fx = _mm256_mul_ps(x, cephes_LOG2EF);
        fx = _mm256_add_ps(fx, p5);

        tmp = _mm256_floor_ps(fx);

        cmask = _mm256_cmp_ps(tmp, fx, _CMP_GT_OS);
        cmask = _mm256_and_ps(cmask, one);
        fx = _mm256_sub_ps(tmp, cmask);

        tmp = _mm256_mul_ps(fx, cephes_exp_C1);
        z = _mm256_mul_ps(fx, cephes_exp_C2);
        x = _mm256_sub_ps(x, tmp);
        x = _mm256_sub_ps(x, z);

        z = _mm256_mul_ps(x, x);

        y = cephes_exp_p0;
        y = _mm256_fmadd_ps(y, x, cephes_exp_p1);
        y = _mm256_fmadd_ps(y, x, cephes_exp_p2);
        y = _mm256_fmadd_ps(y, x, cephes_exp_p3);
        y = _mm256_fmadd_ps(y, x, cephes_exp_p4);
        y = _mm256_fmadd_ps(y, x, cephes_exp_p5);
        y = _mm256_fmadd_ps(y, z, x);
        y = _mm256_add_ps(y, one);

        /* build 2^n */
        imm0 = _mm256_cvttps_epi32(fx);

        imm0 = _mm256_slli_epi32(imm0, 23);
        y = _mm256_castsi256_ps(_mm256_and_si256(mask, _mm256_add_epi32(imm0, _mm256_castps_si256(y))));

        // another two AVX2 instructions
        // imm0 = _mm256_add_epi32(imm0, offset);
        // imm0 = _mm256_slli_epi32(imm0, 23);
        // pow2n = _mm256_castsi256_ps(imm0);
        // y = _mm256_mul_ps(y, pow2n);

        _mm256_store_ps(results + i, y);
    }

    return results;
}
#endif


#if defined(__AVX512F__)

static float* exp16_fastest_approx(float* data, size_t num, float* results) {

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

static float* exp16_faster_approx(float* data, size_t num, float* results) {

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

static float* exp16_fast_approx(float* data, size_t num, float* results) {

    __m512  x, fx, tmp, cmask, z, y, pow2n;
    __m512i imm0;

    const __m512 one = _mm512_set1_ps(1.f);
    const __m512 p5 = _mm512_set1_ps(0.5);
    const __m512 max_in = _mm512_set1_ps(88.3762626647949f);
    const __m512 min_in = _mm512_set1_ps(-88.3762626647949f);

    const __m512 cephes_LOG2EF = _mm512_set1_ps(1.44269504088896341);
    const __m512 cephes_exp_C1 = _mm512_set1_ps(0.693359375);
    const __m512 cephes_exp_C2 = _mm512_set1_ps(-2.12194440e-4);

    const __m512 cephes_exp_p0 = _mm512_set1_ps(1.9875691500E-4);
    const __m512 cephes_exp_p1 = _mm512_set1_ps(1.3981999507E-3);
    const __m512 cephes_exp_p2 = _mm512_set1_ps(8.3334519073E-3);
    const __m512 cephes_exp_p3 = _mm512_set1_ps(4.1665795894E-2);
    const __m512 cephes_exp_p4 = _mm512_set1_ps(1.6666665459E-1);
    const __m512 cephes_exp_p5 = _mm512_set1_ps(5.0000001201E-1);
    const __m512i offset = _mm512_set1_epi32(0x7f);
    const __m512i mask = _mm512_set1_epi32(0x7fffffff);


    for (size_t i = 0; i < num; i += 16) {
        x = _mm512_loadu_ps(data + i);

        x = _mm512_min_ps(x, max_in);
        x = _mm512_max_ps(x, min_in);

        /* express exp(x) as exp(g + n*log(2)) */
        fx = _mm512_mul_ps(x, cephes_LOG2EF);
        fx = _mm512_add_ps(fx, p5);

        tmp = _mm512_floor_ps(fx);

        __mmask16 cmask = _mm512_cmp_ps_mask(tmp, fx, _CMP_GT_OS);
        fx = _mm512_mask_sub_ps(tmp, cmask, tmp, one);

        tmp = _mm512_mul_ps(fx, cephes_exp_C1);
        z = _mm512_mul_ps(fx, cephes_exp_C2);
        x = _mm512_sub_ps(x, tmp);
        x = _mm512_sub_ps(x, z);

        z = _mm512_mul_ps(x, x);

        y = cephes_exp_p0;
        y = _mm512_fmadd_ps(y, x, cephes_exp_p1);
        y = _mm512_fmadd_ps(y, x, cephes_exp_p2);
        y = _mm512_fmadd_ps(y, x, cephes_exp_p3);
        y = _mm512_fmadd_ps(y, x, cephes_exp_p4);
        y = _mm512_fmadd_ps(y, x, cephes_exp_p5);
        y = _mm512_fmadd_ps(y, z, x);
        y = _mm512_add_ps(y, one);

        /* build 2^n */
        imm0 = _mm512_cvttps_epi32(fx);

        imm0 = _mm512_slli_epi32(imm0, 23);
        y = _mm512_castsi512_ps(_mm512_and_si512(mask, _mm512_add_epi32(imm0, _mm512_castps_si512(y))));

        // another two AVX2 instructions
        // imm0 = _mm512_add_epi32(imm0, offset);
        // imm0 = _mm512_slli_epi32(imm0, 23);
        // pow2n = _mm512_castsi512_ps(imm0);
        // y = _mm512_mul_ps(y, pow2n);

        _mm512_store_ps(results + i, y);
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
    exp8_fastest_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX2 Fastest] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 1
    gettimeofday(&start, NULL);
    exp8_faster_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX2 Faster] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 0
    gettimeofday(&start, NULL);
    exp8_fast_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX2 Fast] value: e^(%f) = %f, time(ms): %f\n", data[10], results[10], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif
#endif

#if defined(__AVX512F__)
#if 0
    gettimeofday(&start, NULL);
    exp16_fastest_approx(data, num_elms, results);
    gettimeofday(&end, NULL);

    printf("[AVX3 Fastest] value: e^(%f) = %f, time(ms): %f\n", data[1], results[1], TIME_DIFF(start, end) / 1000.0 / loop);
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

