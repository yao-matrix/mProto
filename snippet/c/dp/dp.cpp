#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
#include <sys/time.h>
#include <inttypes.h>
#include <malloc.h> 
#include <string.h>


#define TIME_DIFF(a,b) ((b.tv_sec - a.tv_sec) * 1000000 + (b.tv_usec - a.tv_usec))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static inline float horizontal_add_v256(__m256 v) {
    __m256 x1 = _mm256_hadd_ps(v, v);
    __m256 x2 = _mm256_hadd_ps(x1, x1);
    __m128 x3 = _mm256_extractf128_ps(x2, 1);

    __m128 x4 = _mm_add_ss(_mm256_castps256_ps128(x2), x3);

    return _mm_cvtss_f32(x4);
}


static void dotp_naive(const float *lhs, const float *rhs, size_t dim, float *score) {

    float res = 0;
    for (size_t i = 0; i < dim; i++) {
        res += lhs[i] * rhs[i];
    }

    *score = res;
    return;
}


#if defined(__AVX2__) 
static void dotp_avx2(const float *lhs, const float *rhs, size_t dim, float *score) {
    __m256 ymm_sum0 = _mm256_setzero_ps();
    __m256 ymm_sum1 = _mm256_setzero_ps();


    for (size_t i = 0; i < dim; i += 16) {
        ymm_sum0 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i), _mm256_loadu_ps(rhs + i), ymm_sum0);
        ymm_sum1 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i + 8), _mm256_loadu_ps(rhs + i + 8), ymm_sum1);
    }

    *score = horizontal_add_v256(_mm256_add_ps(ymm_sum0, ymm_sum1));

    return;
}

// qb means query batching
void dotp_avx2_qb(const float *query, const float *base, size_t bs, size_t dim, float *scores) { 
    static __m256 *ymm_sum0 = NULL;
    static __m256 *ymm_sum1 = NULL;

    if (ymm_sum0 == NULL) {
        ymm_sum0 = (__m256*)memalign(32, 2 * bs * sizeof(__m256));
        ymm_sum1 = ymm_sum0 + bs;
    }

    memset(ymm_sum0, 0, 2 * bs * sizeof(__m256));

    for (size_t i = 0; i < dim; i += 16) {
        __m256 v_base0 = _mm256_loadu_ps(base + i);
        __m256 v_base1 = _mm256_loadu_ps(base + i + 8);

        for (size_t j = 0; j < bs; j += 4) {
            ymm_sum0[j] = _mm256_fmadd_ps(v_base0, _mm256_loadu_ps(query + j * dim + i), ymm_sum0[j]);
            ymm_sum1[j] = _mm256_fmadd_ps(v_base1, _mm256_loadu_ps(query + j * dim + i + 8), ymm_sum1[j]);

            ymm_sum0[j + 1] = _mm256_fmadd_ps(v_base0, _mm256_loadu_ps(query + (j + 1) * dim + i), ymm_sum0[j + 1]);
            ymm_sum1[j + 1] = _mm256_fmadd_ps(v_base1, _mm256_loadu_ps(query + (j + 1) * dim + i + 8), ymm_sum1[j + 1]);

            ymm_sum0[j + 2] = _mm256_fmadd_ps(v_base0, _mm256_loadu_ps(query + (j + 2) * dim + i), ymm_sum0[j + 2]);
            ymm_sum1[j + 2] = _mm256_fmadd_ps(v_base1, _mm256_loadu_ps(query + (j + 2) * dim + i + 8), ymm_sum1[j + 2]);
 
            ymm_sum0[j + 3] = _mm256_fmadd_ps(v_base0, _mm256_loadu_ps(query + (j + 3) * dim + i), ymm_sum0[j + 3]);
            ymm_sum1[j + 3] = _mm256_fmadd_ps(v_base1, _mm256_loadu_ps(query + (j + 3) * dim + i + 8), ymm_sum1[j + 3]); 
        }
    }

    for (size_t j = 0; j < bs; j += 4) {
        scores[j] = horizontal_add_v256(_mm256_add_ps(ymm_sum0[j], ymm_sum1[j]));
        scores[j + 1] = horizontal_add_v256(_mm256_add_ps(ymm_sum0[j + 1], ymm_sum1[j + 1]));
        scores[j + 2] = horizontal_add_v256(_mm256_add_ps(ymm_sum0[j + 2], ymm_sum1[j + 2]));
        scores[j + 3] = horizontal_add_v256(_mm256_add_ps(ymm_sum0[j + 3], ymm_sum1[j + 3]));
    }

    // free(ymm_sum0);
    // ymm_sum0 = NULL;

    return;
}
#endif


#if defined(__AVX512F__)
static inline float horizontal_add_v512(__m512 v) {
    __m256 low = _mm512_castps512_ps256(v);
    __m256 high = _mm256_castpd_ps(_mm512_extractf64x4_pd(_mm512_castps_pd(v), 1));

    return horizontal_add_v256(low + high);
}

void dotp_avx3(const float *lhs, const float *rhs, size_t dim, float *score) {
    __m512 zmm_sum0 = _mm512_setzero_ps();
    __m512 zmm_sum1 = _mm512_setzero_ps();


    for (size_t i = 0; i < dim; i += 32) {
        zmm_sum0 = _mm512_fmadd_ps(_mm512_loadu_ps(lhs + i), _mm512_loadu_ps(rhs + i), zmm_sum0);
        zmm_sum1 = _mm512_fmadd_ps(_mm512_loadu_ps(lhs + i + 16), _mm512_loadu_ps(rhs + i + 16), zmm_sum1);
    }

    *score = horizontal_add_v512(_mm512_add_ps(zmm_sum0, zmm_sum1));

    return;
}

void dotp_avx3_qb(const float *query, const float *base, size_t bs, size_t dim, float *scores) {
    static __m512 *zmm_sum0 = NULL;
    static __m512 *zmm_sum1 = NULL;
 
    if (unlikely(zmm_sum0 == NULL)) {
        zmm_sum0 = (__m512*)memalign(64, 2 * bs * sizeof(__m512));
        zmm_sum1 = zmm_sum0 + bs;
    }
    memset(zmm_sum0, 0, 2 * bs * sizeof(__m512));


    // _mm_prefetch((char const*)(base), _MM_HINT_T0);
    // _mm_prefetch((char const*)(base + 16), _MM_HINT_T0);


    __m512 v_base0, v_base1;
    __m512 v_q0, v_q1;

    for (size_t i = 0; i < dim; i += 32) {
        v_base0 = _mm512_loadu_ps(base + i);
        v_base1 = _mm512_loadu_ps(base + i + 16);

        for (size_t j = 0; j < bs; j++) {    
            v_q0 = _mm512_loadu_ps(query + j * dim + i);
            v_q1 = _mm512_loadu_ps(query + j * dim + i + 16);

            zmm_sum0[j] = _mm512_fmadd_ps(v_base0, v_q0, zmm_sum0[j]);
            zmm_sum1[j] = _mm512_fmadd_ps(v_base1, v_q1, zmm_sum1[j]);
        }

        // _mm_prefetch((char const*)(base + i + 32), _MM_HINT_T0);
        // _mm_prefetch((char const*)(base + i + 48), _MM_HINT_T0);
    }

    for (size_t j = 0; j < bs; j++) {
        scores[j] = horizontal_add_v512(_mm512_add_ps(zmm_sum0[j], zmm_sum1[j]));
    }

    // free(zmm_sum0);
    // zmm_sum0 = NULL;

    return;
}

static inline __m512 _mm512_extload_ps(float const* daddr) {
    __m128 d = _mm_load_ps(daddr);

    return _mm512_broadcastss_ps(d);
}

void dotp_avx3_block(const float *query, const float *base, size_t bs, size_t dim, float *scores) {
    static __m512 *zmm_sum0 = NULL;
 
    if (zmm_sum0 == NULL) {
        zmm_sum0 = (__m512*)memalign(64, bs * sizeof(__m512));
    }
    memset(zmm_sum0, 0, 2 * bs * sizeof(__m512));


    // _mm_prefetch((char const*)(base), _MM_HINT_T0);
    // _mm_prefetch((char const*)(base + 16), _MM_HINT_T0);


    __m512 v_base0;
    __m512 v_q0, v_q1;

    for (size_t i = 0; i < dim; i += 1) {
        v_base0 = _mm512_loadu_ps(base + i * 16);

        for (size_t j = 0; j < bs; j += 2) {
            v_q0 = _mm512_extload_ps(query + j * dim + i);
            v_q1 = _mm512_extload_ps(query + (j + 1) * dim + i);

            zmm_sum0[j]     = _mm512_fmadd_ps(v_base0, v_q0, zmm_sum0[j]);
            zmm_sum0[j + 1] = _mm512_fmadd_ps(v_base0, v_q1, zmm_sum0[j + 1]);


            // _mm_prefetch(query + (j + 2) * dim, _MM_HINT_T1);
            // _mm_prefetch(query + (j + 3) * dim, _MM_HINT_T1);
        }

        _mm_prefetch(base + (i + 1) * 16, _MM_HINT_T0);
    }

    for (size_t j = 0; j < bs; j++) {
        _mm512_store_ps(scores + j * 16, zmm_sum0[j]); 
        // _mm512_stream_ps(scores + j * 16, zmm_sum0[j]);
   }

    // free(zmm_sum0);
    // zmm_sum0 = NULL;

    return;
}


#endif


int main(int argc, char *argv[]) {
    size_t base_vec_num = 1024 * 1024; // 1M vectors
    size_t bs = 32;
    size_t vec_dim = 128;

    float  *query  = NULL;
    float  *base   = NULL;
    float  *scores = NULL;

    size_t align_size = 64;

    struct timeval start, end;

    size_t base_elms = base_vec_num * vec_dim;

    base = (float*)memalign(align_size, sizeof(float) * base_elms);
    query = (float*)memalign(align_size, sizeof(float) * vec_dim * bs);
    scores = (float*)memalign(align_size, sizeof(float) * bs * base_vec_num);

    size_t loop = 10;

#if 1
    // fill buffers
    for (size_t i = 0; i < base_elms; i++) {
        base[i] = i / 10000.f;
    }

    for (size_t i = 0; i < bs; i++) {
        for (size_t j = 0; j < vec_dim; j++) {
            query[i * vec_dim + j] = j;
        }
    }
#endif


#if 0
    // warm up
    for (size_t i = 0; i < bs; i++) {
        for (size_t j = 0; j < base_vec_num; j++) {
            dotp_naive(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
        }
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < bs; i++) {
            for (size_t j = 0; j < base_vec_num; j++) {
                dotp_naive(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
            }
        }
    }
    gettimeofday(&end, NULL);

    printf("[Naive baseline] value: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif



#if defined(__AVX2__)
#if 0
    // warm up
    for (size_t i = 0; i < bs; i++) {
        for (size_t j = 0; j < base_vec_num; j++) {
            dotp_avx2(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
        }
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < bs; i++) {
            for (size_t j = 0; j < base_vec_num; j++) {
                dotp_avx2(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
            }
        }
    }
    gettimeofday(&end, NULL);

    printf("[AVX2 baseline] value: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 0
    // warm up
    for (size_t i = 0; i < base_vec_num; i++) {
        dotp_avx2_qb(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < base_vec_num; i++) {
            dotp_avx2_qb(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
        }
    }
    gettimeofday(&end, NULL);

    printf("[AVX2 batch] value: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif
#endif

#if defined(__AVX512F__)

#if 0
    // warm up
    for (size_t i = 0; i < bs; i++) {
        for (size_t j = 0; j < base_vec_num; j++) {
            dotp_avx3(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
        }
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < bs; i++) {
            for (size_t j = 0; j < base_vec_num; j++) {
                dotp_avx3(query + i * vec_dim, base + j * vec_dim, vec_dim, &scores[i * base_vec_num + j]);
            }
        }
    }

    gettimeofday(&end, NULL);
    printf("[AVX3 baseline] value: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 0
    // warm up
    for (size_t i = 0; i < base_vec_num; i++) {
        dotp_avx3_qb(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < base_vec_num; i++) {
            dotp_avx3_qb(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
        }
    }
    gettimeofday(&end, NULL);
    printf("[AVX3 batch] result: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#if 1
    // warm up
    for (size_t i = 0; i < base_vec_num; i += 16) {
        dotp_avx3_block(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
    }

    gettimeofday(&start, NULL);
    for (size_t iter = 0; iter < loop; iter++) {
        for (size_t i = 0; i < base_vec_num; i += 16) {
            dotp_avx3_block(query, base + i * vec_dim, bs, vec_dim, scores + i * bs);
        }
    }
    gettimeofday(&end, NULL);
    printf("[AVX3 block] result: %f, time(ms): %f\n", scores[0], TIME_DIFF(start, end) / 1000.0 / loop);
    fflush(stdout);
#endif

#endif

    free(base);
    base = NULL;
    free(query);
    query = NULL;		
    free(scores);
    scores = NULL;


    return 0;
}

