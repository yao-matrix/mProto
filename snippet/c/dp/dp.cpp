#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
#include <sys/time.h>
#include <inttypes.h>
#include <malloc.h> 

#define TIME_DIFF(a,b) ((b.tv_sec - a.tv_sec) * 1000000 + (b.tv_usec - a.tv_usec))
#define BS 4
#define VECTOR_DIM 128


static inline float horizontal_add_v256(__m256 v) {
    __m256 x1 = _mm256_hadd_ps(v, v);
    __m256 x2 = _mm256_hadd_ps(x1, x1);
    __m128 x3 = _mm256_extractf128_ps(x2, 1);

    __m128 x4 = _mm_add_ss(_mm256_castps256_ps128(x2), x3);

    return _mm_cvtss_f32(x4);
}


// #if defined(__AVX2__) 
#if 1

#if 1
void dotp_avx2(const float *lhs, const float *rhs, size_t size, float *score) {
    __m256 ymm_sum0 = _mm256_setzero_ps();
    __m256 ymm_sum1 = _mm256_setzero_ps();


    for (size_t i = 0; i < size; i += 16) {
        ymm_sum0 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i), _mm256_loadu_ps(rhs + i), ymm_sum0);
        ymm_sum1 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i + 8), _mm256_loadu_ps(rhs + i + 8), ymm_sum1);
    }

    *score = horizontal_add_v256(_mm256_add_ps(ymm_sum0, ymm_sum1));

    return;
}
#else
void dotp_avx2(const float *lhs, const float *rhs, size_t size, float *score) {
    __m256 ymm_sum0 = _mm256_setzero_ps();
    __m256 ymm_sum1 = _mm256_setzero_ps();


    for (size_t i = 0; i < size; i += 16) {
        ymm_sum0 = _mm256_fmadd_ps(_mm256_castsi256_ps(_mm256_stream_load_si256((__m256i*)(lhs + i))), _mm256_loadu_ps(rhs + i), ymm_sum0);
        ymm_sum1 = _mm256_fmadd_ps(_mm256_castsi256_ps(_mm256_stream_load_si256((__m256i*)(lhs + i + 8))), _mm256_loadu_ps(rhs + i + 8), ymm_sum1);
    }

    *score = horizontal_add_v256(_mm256_add_ps(ymm_sum0, ymm_sum1));

    return;
}
#endif


void dotp4_avx2(const float *base, const float *query, size_t offset, size_t size, float *score) {
    __m256 ymm_sum_0[BS] = {_mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps()};
    __m256 ymm_sum_1[BS] = {_mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps()};

    // size_t index = offset / size;

    for (size_t i = 0; i < size; i += 16) {
        __m256 v_base_0 = _mm256_loadu_ps(base + offset + i);
        __m256 v_base_1 = _mm256_loadu_ps(base + offset + i + 8);

        for (size_t j = 0; j < BS; j++) {
            ymm_sum_0[j] = _mm256_fmadd_ps(v_base_0, _mm256_loadu_ps(query + j * size + i), ymm_sum_0[j]);
            ymm_sum_1[j] = _mm256_fmadd_ps(v_base_1, _mm256_loadu_ps(query + j * size + i + 8), ymm_sum_1[j]);
        }
    }

    for (size_t j = 0; j < BS; j++) {
        score[j] = horizontal_add_v256(_mm256_add_ps(ymm_sum_0[j], ymm_sum_1[j]));
    }

    return;
}
#endif


#if defined(__AVX512F__)
static inline float horizontal_add_v512(__m512 v) {
    __m256 low = _mm512_castps512_ps256(v);
    __m256 high = _mm256_castpd_ps(_mm512_extractf64x4_pd(_mm512_castps_pd(v), 1));

    return horizontal_add_v256(low + high);
}

#if 1
void dotp_avx3(const float *lhs, const float *rhs, size_t size, float *score) {

    __m512 zmm_sum0 = _mm512_setzero_ps();
    __m512 zmm_sum1 = _mm512_setzero_ps();


    for (size_t i = 0; i < size; i += 32) {
        zmm_sum0 = _mm512_fmadd_ps(_mm512_loadu_ps(lhs + i), _mm512_loadu_ps(rhs + i), zmm_sum0);
        zmm_sum1 = _mm512_fmadd_ps(_mm512_loadu_ps(lhs + i + 16), _mm512_loadu_ps(rhs + i + 16), zmm_sum1);
    }

    *score = horizontal_add_v512(_mm512_add_ps(zmm_sum0, zmm_sum1));

    return;
}
#else
void dotp_avx3(const float *lhs, const float *rhs, size_t size, float *score) {
    __m512 zmm_sum0 = _mm512_setzero_ps();
    __m512 zmm_sum1 = _mm512_setzero_ps();


    for (size_t i = 0; i < size; i += 32) {
        zmm_sum0 = _mm512_fmadd_ps(_mm512_castsi512_ps(_mm512_stream_load_si512((__m512i*)(lhs + i))), _mm512_loadu_ps(rhs + i), zmm_sum0);
        zmm_sum1 = _mm512_fmadd_ps(_mm512_castsi512_ps(_mm512_stream_load_si512((__m512i*)(lhs + i + 16))), _mm512_loadu_ps(rhs + i + 16), zmm_sum1);
    }

    *score = horizontal_add_v512(_mm512_add_ps(zmm_sum0, zmm_sum1));

    return;
}
#endif


void dotp4_avx3(const float *base, float *query, size_t offset, size_t size, float *score) {

    __m512 zmm_sum_0[BS] = {_mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps()};
    __m512 zmm_sum_1[BS] = {_mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps()};

    // size_t index = offset / size;

    for (size_t i = 0; i < size; i += 32) {
        __m512 v_base_0 = _mm512_loadu_ps(base + offset + i);
        __m512 v_base_1 = _mm512_loadu_ps(base + offset + i + 16);

        for (size_t j = 0; j < BS; j++) {
            zmm_sum_0[j] = _mm512_fmadd_ps(v_base_0, _mm512_loadu_ps(query + j * size + i), zmm_sum_0[j]);
            zmm_sum_1[j] = _mm512_fmadd_ps(v_base_1, _mm512_loadu_ps(query + j * size + i + 16), zmm_sum_1[j]);
        }
    }

    for (size_t j = 0; j < BS; j++) {
        score[j] = horizontal_add_v512(_mm512_add_ps(zmm_sum_0[j], zmm_sum_1[j]));
    }

    return;
}
#endif

int main(int argc, char *argv[]) {
    size_t base_vec_num = 1024 * 1024 * 8; // 8M vectors

    float  *query    = NULL;
    float  *base     = NULL;
    float  score[BS] = {0};

    size_t align_size = 64;

    size_t base_elms = base_vec_num * VECTOR_DIM;

    struct timeval start, end;

    base = (float*)memalign(align_size, sizeof(float) * base_elms);
    query = (float*)memalign(align_size, sizeof(float) * VECTOR_DIM * BS);

#if 1
    // fill buffers
    for (size_t i = 0; i < base_elms; i++) {
        base[i] = 1.0;
    }

    for (size_t i = 0; i < BS; i++) {
        for (size_t j = 0; j < VECTOR_DIM; j++) {
            query[i * VECTOR_DIM + j] = j;
        }
    }
#endif

#if defined(__AVX2__)

#if 0
    // warm up
    for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
        for (size_t j = 0; j < BS; j++) {
            dotp_avx2(base + i, query + j * VECTOR_DIM, VECTOR_DIM, &score[j]);
        }
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < 10; iter++) {
        for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
            for (size_t j = 0; j < BS; j++) {
                dotp_avx2(base + i, query + j * VECTOR_DIM, VECTOR_DIM, &score[j]);
                // if (i + VECTOR_DIM < base_elms) {
                // _mm_prefetch(base + i + VECTOR_DIM, _MM_HINT_NTA);
                // }
            }
        }
    }

    gettimeofday(&end, NULL);
    printf("[AVX2 baseline] result: %f, time(ms): %f\n", score[0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif

#if 0
    // warm up
    for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
        dotp4_avx2(base, query, i, VECTOR_DIM, score);
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < 10; iter++) {
        for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
            // if (i + VECTOR_DIM < base_elms) {
            //    _mm_prefetch(base + i + VECTOR_DIM, _MM_HINT_T0);
            // }
            dotp4_avx2(base, query, i, VECTOR_DIM, score);
        }
    }
    gettimeofday(&end, NULL);
    printf("[AVX2 batch] result: %f, time(ms): %f\n", score[0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif
#endif

#if defined(__AVX512F__)

#if 1
    // warm up
    for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
        for (size_t j = 0; j < BS; j++) {
            dotp_avx3(base + i, query + j * VECTOR_DIM, VECTOR_DIM, &score[j]);
        }
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < 10; iter++) {
        for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
            for (size_t j = 0; j < BS; j++) {
                dotp_avx3(base + i, query + j * VECTOR_DIM, VECTOR_DIM, &score[j]);
            }
        }
    }

    gettimeofday(&end, NULL);
    printf("[AVX3 baseline] result: %f, time(ms): %f\n", score[0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif

#if 0
    // warm up
    for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
        dotp4_avx3(base, query, i, VECTOR_DIM, score);
    }
 
    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < 10; iter++) {
        for (size_t i = 0; i < base_elms; i += VECTOR_DIM) {
            dotp4_avx3(base, query, i, VECTOR_DIM, score);
        }
    }
    gettimeofday(&end, NULL);
    printf("[AVX3 batch] result: %f, time(ms): %f\n", score[0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif


#endif

    free(base);
    base = NULL;
    free(query);
    query = NULL;		

    return 0;
}

