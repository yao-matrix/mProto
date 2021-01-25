#include "mkldnn.hpp"
#include "bfloat16.hpp"
#include "mkldnn_debug.h"

#include "mkldnn.h"

#include <iostream>
#include <iomanip>
#include <chrono>

#include <mkl.h>
#include <Eigen/Dense>

//#define XBYAK_NO_OP_NAMES
//#include <xbyak.h>

typedef mkldnn::impl::bfloat16_t bfloat16;

void init_param(int m, int n, int k, float *A, float *B, float *C, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16, Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat);

double test_eigen_sgemm(Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat, int m, int n, int k);
double test_mkl_sgemm(float *A, float *B, float *C, int m, int n, int k);
double test_mkl_sgemm_transB(float *A, float *B, float *C, int m, int n, int k);
double test_mkldnn_sgemm(float *A, float *B, float *C, int m, int n, int k);
double test_mkldnn_gemm_bf16bf16f32(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_gemm_bf16bf16f32_transB(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_gemm_bf16bf16f32_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_gemm_bf16bf16f32_transB_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_gemm_bf16bf16f32_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_gemm_bf16bf16f32_transB_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_mkldnn_omp_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
//double test_jit_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);

int main(int argc, char *argv[])
{
    printf("./gemmbench m n k\nargc = %d\n", argc);
    for(int ndx = 0; ndx != argc; ++ndx)
        printf("argv[%d] --> %s\n", ndx, argv[ndx]);

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);

    bfloat16 *A_bf16 = new bfloat16[m*k];
    bfloat16 *B_bf16 = new bfloat16[k*n];
    bfloat16 *C_bf16 = new bfloat16[m*n];

    float *A = new float[m*k];
    float *B = new float[k*n];
    float *C = new float[m*n];

    Eigen::MatrixXf A_mat(m, k);
    Eigen::MatrixXf B_mat(k, n);
    Eigen::MatrixXf C_mat(m, n);

    init_param(m, n, k, A, B, C, A_bf16, B_bf16, C_bf16, A_mat, B_mat, C_mat);
    std::cout << "\nstarting..." << std::endl;

    double t_eigen_sgemm  = test_eigen_sgemm(A_mat, B_mat, C_mat, m, n, k);

    double t_mkl_sgemm    = test_mkl_sgemm(A, B, C, m, n, k);
    double t_mkl_sgemm_tB = test_mkl_sgemm_transB(A, B, C, m, n, k);

    double t_mkldnn_sgemm = test_mkldnn_sgemm(A, B, C, m, n, k);
    double t_mkldnn_gemm_bf16 = test_mkldnn_gemm_bf16bf16f32(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_gemm_bf16_tB = test_mkldnn_gemm_bf16bf16f32_transB(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_gemm_bf16_cvt = test_mkldnn_gemm_bf16bf16f32_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_gemm_bf16_tB_cvt = test_mkldnn_gemm_bf16bf16f32_transB_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_gemm_bf16_omp_cvt = test_mkldnn_gemm_bf16bf16f32_omp_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_gemm_bf16_tB_omp_cvt = test_mkldnn_gemm_bf16bf16f32_transB_omp_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_cvt     = test_mkldnn_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_mkldnn_omp_cvt = test_mkldnn_omp_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    //double t_mkldnn_jit_cvt = test_jit_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);

    printf("\n>> omp num_procs: %d\n", omp_get_num_procs());
    printf("eigen gemm: \t%.6f\n", t_eigen_sgemm);
    printf("mkl sgemm: \t%.6f ms --> baseline\n", t_mkl_sgemm);
    printf("mkl sgemm+transB:            \t%.6f \t+%.3fX\n", t_mkl_sgemm_tB,                t_mkl_sgemm/t_mkl_sgemm_tB);
    printf("mkldnn sgemm:                \t%.6f \t+%.3fX\n", t_mkldnn_sgemm,                t_mkl_sgemm/t_mkldnn_sgemm);
    printf("mkldnn bgemm:                \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16,            t_mkl_sgemm/t_mkldnn_gemm_bf16);
    printf("mkldnn bgemm+transB:         \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16_tB,         t_mkl_sgemm/t_mkldnn_gemm_bf16_tB);
    printf("mkldnn bgemm+cvt:            \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16_cvt,        t_mkl_sgemm/t_mkldnn_gemm_bf16_cvt);
    printf("mkldnn bgemm+transB+cvt:     \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16_tB_cvt,     t_mkl_sgemm/t_mkldnn_gemm_bf16_tB_cvt);
    printf("mkldnn bgemm+omp_cvt:        \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16_omp_cvt,    t_mkl_sgemm/t_mkldnn_gemm_bf16_omp_cvt);
    printf("mkldnn bgemm+transB+omp_cvt: \t%.6f \t+%.3fX\n", t_mkldnn_gemm_bf16_tB_omp_cvt, t_mkl_sgemm/t_mkldnn_gemm_bf16_tB_omp_cvt);
    printf("mkldnn cvt:     \t%.6f \tt/bgemm:   %.3f%\n", t_mkldnn_cvt,     t_mkldnn_cvt/t_mkldnn_gemm_bf16*100);
    printf("mkldnn omp_cvt: \t%.6f \tt/bgemm:   %.3f%\n", t_mkldnn_omp_cvt, t_mkldnn_omp_cvt/t_mkldnn_gemm_bf16*100);
    //printf("mkldnn jit_cvt: \t%.6f \tt/bgemm:   %.3f%\n", t_mkldnn_jit_cvt, t_mkldnn_jit_cvt/t_mkldnn_gemm_bf16*100);

    delete[] A_bf16;
    delete[] B_bf16;
    delete[] C_bf16;

    delete[] A;
    delete[] B;
    delete[] C;

    return 0;
}


void init_param(int m, int n, int k, float *A, float *B, float *C, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16, Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat)
{
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < k; ++j) {
            A_bf16[i*k+j] = (bfloat16)1.1;
            A[i*k+j] = 1.1;
            A_mat.row(i).col(j) << 1.1;
        }
    }

    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < n; ++j) {
            B_bf16[i*n+j] = (bfloat16)1.1;
            B[i*n+j] = 1.1;
            B_mat.row(i).col(j) << 1.1;
        }
    }

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            C_bf16[i*n+j] = (bfloat16)1.1;
            C[i*n+j] = 1.1;
            C_mat.row(i).col(j) << 1.1;
        }
    }
}

double test_eigen_sgemm(Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat, int m, int n, int k)
{
    C_mat = A_mat * B_mat;

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        C_mat = A_mat * B_mat;
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_mat(0, 0) << "," << C_mat(m-1, n-1) << std::endl;

    return tag_diff;
}

double test_mkl_sgemm(float *A, float *B, float *C, int m, int n, int k)
{
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0, A, k, B, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0, A, k, B, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkl_sgemm_transB(float *A, float *B, float *C, int m, int n, int k)
{
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, m, n, k, 1.0, A, k, B, k, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, m, n, k, 1.0, A, k, B, k, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_sgemm(float *A, float *B, float *C, int m, int n, int k)
{
    mkldnn_sgemm('N', 'N', m, n, k, 1.0, A, k, B, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn_sgemm('N', 'N', m, n, k, 1.0, A, k, B, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32_transB(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }
    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
        mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32_transB_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{

    mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
        mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
	mkldnn_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_gemm_bf16bf16f32_transB_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
	mkldnn_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_mkldnn_omp_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            mkldnn::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}
/*
struct Code : Xbyak::CodeGenerator {
    const Xbyak::Reg64& src;
    const Xbyak::Reg64& dst;
    const Xbyak::Reg32& loop;
    Code()
        : src(rsi)
        , dst(rdi)
        , loop(edx)
    {
        Xbyak::Label l0;
        L(l0);

        vcvtneps2bf16(ymm0, zword[src]);
        vmovups(yword[dst], ymm0);
        add(src, 64);
        add(dst, 32);

        dec(loop);
        jg(l0, T_NEAR);

        mov(eax, loop);
        ret();
    }
};

double test_jit_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    Code c;
    int (*f)(void*, void*, int) = c.getCode<int (*)(void*, void*, int)>();
    int num = m*k/16;
    int ret = f(A_bf16, A, num);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        f(A_bf16, A, num);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}
*/
