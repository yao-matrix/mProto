#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
#include <sys/time.h>
#include <inttypes.h>
#include <malloc.h> 
 
static inline float horizontal_add_v256(__m256 v) {
    __m256 x1 = _mm256_hadd_ps(v, v);
    __m256 x2 = _mm256_hadd_ps(x1, x1);
    __m128 x3 = _mm256_extractf128_ps(x2, 1);

    __m128 x4 = _mm_add_ss(_mm256_castps256_ps128(x2), x3);

    return _mm_cvtss_f32(x4);
}


void dotp_avx2(const float *lhs, const float *rhs, size_t size, float *result) {
    __m256 ymm_sum1 = _mm256_setzero_ps();
    __m256 ymm_sum2 = _mm256_setzero_ps();

    for (int i = 0; i < size; i += 16) {
        ymm_sum1 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i), _mm256_loadu_ps(rhs + i), ymm_sum1);
        ymm_sum2 = _mm256_fmadd_ps(_mm256_loadu_ps(lhs + i + 8), _mm256_loadu_ps(rhs + i + 8), ymm_sum2);
    }

    *result = horizontal_add_v256(_mm256_add_ps(ymm_sum1, ymm_sum2));

    return;
}


void dotp4_avx2(const float *base, float *array[4], size_t offset, size_t size, float **result) {
    __m256 ymm_sum_0[4] = {_mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps()};
    __m256 ymm_sum_1[4] = {_mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps()};

    size_t index = offset / size;

    for (int i = 0; i < size; i += 16) {
        __m256 v_base_0 = _mm256_loadu_ps(base + offset + i);
        __m256 v_base_1 = _mm256_loadu_ps(base + offset + i + 8);

        for (int j = 0; j < 4; j++) {
            ymm_sum_0[j] = _mm256_fmadd_ps(v_base_0, _mm256_loadu_ps(array[j] + offset + i), ymm_sum_0[j]);
            ymm_sum_1[j] = _mm256_fmadd_ps(v_base_1, _mm256_loadu_ps(array[j] + offset + i + 8), ymm_sum_1[j]);
        }
    }

    for (int j = 0; j < 4; j++) {
        result[j][index] = horizontal_add_v256(_mm256_add_ps(ymm_sum_0[j], ymm_sum_1[j]));
    }

    return;
}

#if defined(__AVX512F__)
static inline float horizontal_add_v512(__m512 v) {
    __m256 low = _mm512_castps512_ps256(v);
    __m256 high = _mm256_castpd_ps(_mm512_extractf64x4_pd(_mm512_castps_pd(v), 1));

    return horizontal_add_v256(low) + horizontal_add_v256(high);
}

void dotp4_avx3(const float *base, float *array[4], size_t offset, size_t size, float **result) {

    __m512 zmm_sum_0[4] = {_mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps()};
    __m512 zmm_sum_1[4] = {_mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps(), _mm512_setzero_ps()};

    size_t index = offset / size;

    for (int i = 0; i < size; i += 32) {
        __m512 v_base_0 = _mm512_loadu_ps(base + offset);
        __m512 v_base_1 = _mm512_loadu_ps(base + offset + 16);

        for (int j = 0; j < 4; j++) {
            zmm_sum_0[j] = _mm512_fmadd_ps(v_base_0, _mm512_loadu_ps(array[j] + offset + i), zmm_sum_0[j]);
            zmm_sum_1[j] = _mm512_fmadd_ps(v_base_1, _mm512_loadu_ps(array[j] + offset + i + 16), zmm_sum_1[j]);
        }
    }

    for (int j = 0; j < 4; j++) {
        result[j][index] = horizontal_add_v512(_mm512_add_ps(zmm_sum_0[j], zmm_sum_1[j]));
    }

    return;
}
#endif

#define TIME_DIFF(a,b) ((b.tv_sec - a.tv_sec) * 1000000 + (b.tv_usec - a.tv_usec))
#define BS 4
#define VECTOR_DIM 128

int main(int argc, char *argv[]) {
    int vec_num = 1024 * 1024 * 8;

    float *array[BS] = {NULL};
    float *result[BS] = {NULL};
    float *base_array = NULL;
    int align_size = 64;

    struct timeval start, end;

    base_array = (float*)memalign(align_size, sizeof(float) * vec_num * VECTOR_DIM);
    for (int i = 0; i < BS; i++) {
        array[i] = (float*)memalign(align_size, sizeof(float) * vec_num * VECTOR_DIM);
        result[i] = (float*)memalign(align_size, sizeof(float) * vec_num);
    }

    for (int i = 0; i < vec_num; i++) {
        base_array[i] = 1.0;
        for (int j = 0; j < BS; j++) {
            array[j][i] = i;
        }
    }

    int vec_elms = vec_num * VECTOR_DIM;

#if 1
    gettimeofday(&start, NULL);
    for (int i = 0; i < vec_elms; i += VECTOR_DIM) {
        size_t index = i / VECTOR_DIM;
        for (int j = 0; j < BS; j++) {
            dotp_avx2(base_array + i, array[j] + i, VECTOR_DIM, result[j] + index);
        }
    }
    gettimeofday(&end, NULL);
    printf("[AVX2 baseline] result: %f, time(ms): %f\n", result[0][0], TIME_DIFF(start,end) / 1000.0);
    fflush(stdout);
#endif

#if 1
    gettimeofday(&start, NULL);
    for (int i = 0; i < vec_elms; i += VECTOR_DIM) {
        dotp4_avx2(base_array, array, i, VECTOR_DIM, result);
    }
    gettimeofday(&end, NULL);
    printf("[AVX2 batch] result: %f, time(ms): %f\n", result[0][0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif


#if defined(__AVX512F__)
    gettimeofday(&start, NULL);
    for (int i = 0; i < vec_elms; i += VECTOR_DIM) {
        dotp4_avx3(base_array, array, i, VECTOR_DIM, result);
    }
    gettimeofday(&end, NULL);
    printf("[AVX3 batch] result: %f, time(ms): %f\n", result[0][0], TIME_DIFF(start, end) / 1000.0);
    fflush(stdout);
#endif

    free(base_array);
    base_array = NULL;
    for (int i = 0; i < BS; i++) {
        free(array[i]);
        array[i] = NULL;

        free(result[i]);
        result[i] = NULL;
    }		

    return 0;
}
