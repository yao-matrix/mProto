#include "dnnl.hpp"
#include "dnnl_debug.h"
#include "bfloat16.hpp"

#include "dnnl.h"

#include <iostream>
#include <iomanip>
#include <chrono>

#include <mkl.h>

#define EIGEN_USE_MKL_ALL
#include <Eigen/Dense>

//#define XBYAK_NO_OP_NAMES
//#include <xbyak.h>

#include "dnnl_common.h"
#include "dnnl_inner_product.h"
#include "dnnl_matmul.h"

void init_param(int m, int n, int k, float *A, float *B, float *C, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16, Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat);

double test_eigen_sgemm(Eigen::MatrixXf& A_mat, Eigen::MatrixXf& B_mat, Eigen::MatrixXf& C_mat, int m, int n, int k);
double test_mkl_sgemm(float *A, float *B, float *C, int m, int n, int k);
double test_mkl_sgemm_transB(float *A, float *B, float *C, int m, int n, int k);
double test_dnnl_sgemm(float *A, float *B, float *C, int m, int n, int k);
double test_dnnl_gemm_bf16bf16f32(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_gemm_bf16bf16f32_transB(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_gemm_bf16bf16f32_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_gemm_bf16bf16f32_transB_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_gemm_bf16bf16f32_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_gemm_bf16bf16f32_transB_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_omp_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
//double test_jit_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_cvt_bfloat16_to_float(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);
double test_dnnl_omp_cvt_bfloat16_to_float(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16);

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k);
template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product_v2(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k);
template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product_eltwise(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k);

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_matmul(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k);
template <typename T_A, typename T_B, typename T_C>
double test_dnnl_matmul2(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int m, int n, int k);
template <typename T_A, typename T_B, typename T_C>
double test_dnnl_matmul2_eltwise(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int m, int n, int k);

template <typename T_A, typename T_B, typename T_C>
double test_dnnl_batchmatmul(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int mb, int m, int n, int k);


int main(int argc, char *argv[])
{
    printf("./gemmbench_dnnl m n k\nargc = %d\n", argc);
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

    double t_dnnl_sgemm = test_dnnl_sgemm(A, B, C, m, n, k);
    double t_dnnl_gemm_bf16 = test_dnnl_gemm_bf16bf16f32(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_gemm_bf16_tB = test_dnnl_gemm_bf16bf16f32_transB(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_gemm_bf16_cvt = test_dnnl_gemm_bf16bf16f32_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_gemm_bf16_tB_cvt = test_dnnl_gemm_bf16bf16f32_transB_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_gemm_bf16_omp_cvt = test_dnnl_gemm_bf16bf16f32_omp_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_gemm_bf16_tB_omp_cvt = test_dnnl_gemm_bf16bf16f32_transB_omp_cvt(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_cvt_f2b     = test_dnnl_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_omp_cvt_f2b = test_dnnl_omp_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    //double t_dnnl_jit_cvt = test_jit_cvt_float_to_bfloat16(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_cvt_b2f     = test_dnnl_cvt_bfloat16_to_float(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);
    double t_dnnl_omp_cvt_b2f = test_dnnl_omp_cvt_bfloat16_to_float(A, B, C, m, n, k, A_bf16, B_bf16, C_bf16);

    engine cpu_engine;
    stream cpu_stream;

    stream stream(eng);
    cpu_engine = eng;
    cpu_stream = stream;

    float *bias = new float[n];
    bfloat16 *bias_bf16 = new bfloat16[n];
    for (int i = 0; i < n; ++i) {
        bias[i] = 1.1;
        bias_bf16[i] = (bfloat16)1.1;
    }

    double t_dnnl_ip_ffff  = test_dnnl_inner_product(cpu_engine, cpu_stream, A, B, bias, C, m, n, k);
    double t_dnnl_ip2_ffff = test_dnnl_inner_product_v2(cpu_engine, cpu_stream, A, B, bias, C, m, n, k);
    double t_dnnl_ip2_fffb = test_dnnl_inner_product_v2(cpu_engine, cpu_stream, A, B, bias, C_bf16, m, n, k);
    double t_dnnl_ip2_fbbb = test_dnnl_inner_product_v2(cpu_engine, cpu_stream, A, B_bf16, bias_bf16, C_bf16, m, n, k);
    double t_dnnl_ip_bbbb  = test_dnnl_inner_product(cpu_engine, cpu_stream, A_bf16, B_bf16, bias_bf16, C_bf16, m, n, k);
    double t_dnnl_ip_bbbb_e= test_dnnl_inner_product_eltwise(cpu_engine, cpu_stream, A_bf16, B_bf16, bias_bf16, C_bf16, m, n, k);
    double t_dnnl_ip_bbbf  = test_dnnl_inner_product(cpu_engine, cpu_stream, A_bf16, B_bf16, bias_bf16, C, m, n, k);
    double t_dnnl_ip_bbff  = test_dnnl_inner_product(cpu_engine, cpu_stream, A_bf16, B_bf16, bias, C, m, n, k);

    double t_dnnl_mm_ffff  = test_dnnl_matmul(cpu_engine, cpu_stream, A, B, bias, C, m, n, k);
    double t_dnnl_mm_bbbb  = test_dnnl_matmul(cpu_engine, cpu_stream, A_bf16, B_bf16, bias_bf16, C_bf16, m, n, k);
    double t_dnnl_mm_bbbf  = test_dnnl_matmul(cpu_engine, cpu_stream, A_bf16, B_bf16, bias_bf16, C, m, n, k);
    double t_dnnl_mm_fff  = test_dnnl_matmul2(cpu_engine, cpu_stream, A, B, C, m, n, k);
    double t_dnnl_mm_bbb  = test_dnnl_matmul2(cpu_engine, cpu_stream, A_bf16, B_bf16, C_bf16, m, n, k);
    double t_dnnl_mm_bbb_e= test_dnnl_matmul2_eltwise(cpu_engine, cpu_stream, A_bf16, B_bf16, C_bf16, m, n, k);
    double t_dnnl_mm_bbf  = test_dnnl_matmul2(cpu_engine, cpu_stream, A_bf16, B_bf16, C, m, n, k);

    int mb = 10;
    bfloat16 *BA_bf16 = new bfloat16[mb*m*k];
    bfloat16 *BB_bf16 = new bfloat16[mb*k*n];
    bfloat16 *BC_bf16 = new bfloat16[mb*m*n];

    for (int i = 0; i < mb*m*k; ++i)
        BA_bf16[i] = (bfloat16)1.1;
    for (int i = 0; i < mb*k*n; ++i)
        BB_bf16[i] = (bfloat16)1.1;
    for (int i = 0; i < mb*m*n; ++i)
        BC_bf16[i] = (bfloat16)1.1;

    double t_dnnl_bmm_bbb = test_dnnl_batchmatmul(cpu_engine, cpu_stream, BA_bf16, BB_bf16, BC_bf16, mb, m, n, k);

    del_dnnl();

    printf("\n>> omp num_procs: %d\n", omp_get_num_procs());
    printf("eigen sgemm:               \t%.6f\n", t_eigen_sgemm);
    printf("mkl sgemm:                 \t%.6f ms --> baseline\n", t_mkl_sgemm);
    printf("mkl sgemm+transB:          \t%.6f \t+%.3fX\n", t_mkl_sgemm_tB,              t_mkl_sgemm/t_mkl_sgemm_tB);
    printf("dnnl sgemm:                \t%.6f \t+%.3fX\n", t_dnnl_sgemm,                t_mkl_sgemm/t_dnnl_sgemm);
    printf("dnnl bgemm:                \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16,            t_mkl_sgemm/t_dnnl_gemm_bf16);
    printf("dnnl bgemm+transB:         \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16_tB,         t_mkl_sgemm/t_dnnl_gemm_bf16_tB);
    printf("dnnl bgemm+cvt:            \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16_cvt,        t_mkl_sgemm/t_dnnl_gemm_bf16_cvt);
    printf("dnnl bgemm+transB+cvt:     \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16_tB_cvt,     t_mkl_sgemm/t_dnnl_gemm_bf16_tB_cvt);
    printf("dnnl bgemm+omp_cvt:        \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16_omp_cvt,    t_mkl_sgemm/t_dnnl_gemm_bf16_omp_cvt);
    printf("dnnl bgemm+transB+omp_cvt: \t%.6f \t+%.3fX\n", t_dnnl_gemm_bf16_tB_omp_cvt, t_mkl_sgemm/t_dnnl_gemm_bf16_tB_omp_cvt);
    printf("dnnl cvt f2b:              \t%.6f \tt/bgemm:   %.3f%\n", t_dnnl_cvt_f2b,     t_dnnl_cvt_f2b/t_dnnl_gemm_bf16*100);
    printf("dnnl omp_cvt f2b:          \t%.6f \tt/bgemm:   %.3f%\n", t_dnnl_omp_cvt_f2b, t_dnnl_omp_cvt_f2b/t_dnnl_gemm_bf16*100);
    //printf("dnnl jit_cvt: \t%.6f \tt/bgemm:   %.3f%\n", t_dnnl_jit_cvt, t_dnnl_jit_cvt/t_dnnl_gemm_bf16*100);
    printf("dnnl cvt b2f:              \t%.6f \tt/bgemm:   %.3f%\n", t_dnnl_cvt_b2f,     t_dnnl_cvt_b2f/t_dnnl_gemm_bf16*100);
    printf("dnnl omp_cvt b2f:          \t%.6f \tt/bgemm:   %.3f%\n", t_dnnl_omp_cvt_b2f, t_dnnl_omp_cvt_b2f/t_dnnl_gemm_bf16*100);

    printf(">> inner_product, f: fp32, b: bf16, elw: eltwise\n");
    printf("dnnl inner_product  ffff:     \t%.6f \t+%.3fX\n", t_dnnl_ip_ffff,   t_mkl_sgemm/t_dnnl_ip_ffff);
    printf("dnnl inner_product2 ffff:     \t%.6f \t+%.3fX\n", t_dnnl_ip2_ffff,  t_mkl_sgemm/t_dnnl_ip2_ffff);
    printf("dnnl inner_product2 fffb:     \t%.6f \t+%.3fX\n", t_dnnl_ip2_fffb,  t_mkl_sgemm/t_dnnl_ip2_fffb);
    printf("dnnl inner_product2 fbbb:     \t%.6f \t+%.3fX\n", t_dnnl_ip2_fbbb,  t_mkl_sgemm/t_dnnl_ip2_fbbb);
    printf("dnnl inner_product  bbbb:     \t%.6f \t+%.3fX\n", t_dnnl_ip_bbbb,   t_mkl_sgemm/t_dnnl_ip_bbbb);
    printf("dnnl inner_product  bbbb+elw: \t%.6f \t+%.3fX\n", t_dnnl_ip_bbbb_e, t_mkl_sgemm/t_dnnl_ip_bbbb_e);
    printf("dnnl inner_product  bbbf:     \t%.6f \t+%.3fX\n", t_dnnl_ip_bbbf,   t_mkl_sgemm/t_dnnl_ip_bbbf);
    printf("dnnl inner_product  bbff:     \t%.6f \t+%.3fX\n", t_dnnl_ip_bbff,   t_mkl_sgemm/t_dnnl_ip_bbff);

    printf(">> matmul, f: fp32, b: bf16, elw: eltwise\n");
    printf("dnnl matmul ffff:          \t%.6f \t+%.3fX\n", t_dnnl_mm_ffff,   t_mkl_sgemm/t_dnnl_mm_ffff);
    printf("dnnl matmul bbbb:          \t%.6f \t+%.3fX\n", t_dnnl_mm_bbbb,   t_mkl_sgemm/t_dnnl_mm_bbbb);
    printf("dnnl matmul bbbf:          \t%.6f \t+%.3fX\n", t_dnnl_mm_bbbf,   t_mkl_sgemm/t_dnnl_mm_bbbf);
    printf("dnnl matmul2 fff:          \t%.6f \t+%.3fX\n", t_dnnl_mm_fff,    t_mkl_sgemm/t_dnnl_mm_fff);
    printf("dnnl matmul2 bbb:          \t%.6f \t+%.3fX\n", t_dnnl_mm_bbb,    t_mkl_sgemm/t_dnnl_mm_bbb);
    printf("dnnl matmul2 bbb+elw:      \t%.6f \t+%.3fX\n", t_dnnl_mm_bbb_e,  t_mkl_sgemm/t_dnnl_mm_bbb_e);
    printf("dnnl matmul2 bbf:          \t%.6f \t+%.3fX\n", t_dnnl_mm_bbf,    t_mkl_sgemm/t_dnnl_mm_bbf);
    printf("dnnl 10 batch matmul bbb:  \t%.6f \t+%.3fX\n", t_dnnl_bmm_bbb/10,t_mkl_sgemm/t_dnnl_bmm_bbb*10);

    delete[] A_bf16;
    delete[] B_bf16;
    delete[] C_bf16;
    delete[] bias_bf16;

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

double test_dnnl_sgemm(float *A, float *B, float *C, int m, int n, int k)
{
    dnnl_sgemm('N', 'N', m, n, k, 1.0, A, k, B, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl_sgemm('N', 'N', m, n, k, 1.0, A, k, B, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32_transB(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }
    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
        dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32_transB_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{

    dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
        dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
	dnnl_gemm_bf16bf16f32('N', 'N', m, n, k, 1.0, A_bf16, k, B_bf16, n, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_gemm_bf16bf16f32_transB_omp_cvt(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
	dnnl_gemm_bf16bf16f32('N', 'T', m, n, k, 1.0, A_bf16, k, B_bf16, k, 0.0, C, n);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl::impl::cvt_float_to_bfloat16(A_bf16, A, m*k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_omp_cvt_float_to_bfloat16(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            dnnl::impl::cvt_float_to_bfloat16(A_bf16+i*k, A+i*k, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_cvt_bfloat16_to_float(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    dnnl::impl::cvt_bfloat16_to_float(A, A_bf16, m*k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        dnnl::impl::cvt_bfloat16_to_float(A, A_bf16, m*k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

double test_dnnl_omp_cvt_bfloat16_to_float(float *A, float *B, float *C, int m, int n, int k, bfloat16 *A_bf16, bfloat16 *B_bf16, bfloat16 *C_bf16)
{
    #pragma omp parallel for num_threads(omp_get_num_procs())
    for (int i = 0; i < m; ++i)
        dnnl::impl::cvt_bfloat16_to_float(A+i*k, A_bf16+i*k, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < m; ++i)
            dnnl::impl::cvt_bfloat16_to_float(A+i*k, A_bf16+i*k, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C[0] << "," << C[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k)
{
    InnerProduct(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        InnerProduct(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product_v2(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k)
{
    InnerProduct_v2(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        InnerProduct_v2(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;
    // std::cout << "result: " << (bfloat16)933.023 << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_inner_product_eltwise(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k)
{
    InnerProduct_eltwise(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        InnerProduct_eltwise(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_bias, typename T_C>
double test_dnnl_matmul(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_bias* bias_buf, T_C* C_buf, int m, int n, int k)
{
    MatMul(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        MatMul(eng, stm, A_buf, B_buf, bias_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_C>
double test_dnnl_matmul2(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int m, int n, int k)
{
    MatMul2(eng, stm, A_buf, B_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        MatMul2(eng, stm, A_buf, B_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_C>
double test_dnnl_matmul2_eltwise(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int m, int n, int k)
{
    MatMul2_eltwise(eng, stm, A_buf, B_buf, C_buf, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        MatMul2_eltwise(eng, stm, A_buf, B_buf, C_buf, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

    return tag_diff;
}

template <typename T_A, typename T_B, typename T_C>
double test_dnnl_batchmatmul(engine eng, stream stm, T_A* A_buf, T_B* B_buf, T_C* C_buf, int mb, int m, int n, int k)
{
    BatchMatMul(eng, stm, A_buf, B_buf, C_buf, mb, m, n, k);

    auto tag_1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        BatchMatMul(eng, stm, A_buf, B_buf, C_buf, mb, m, n, k);
    }

    auto tag_2 = std::chrono::high_resolution_clock::now();
    auto tag_diff = std::chrono::duration<double>(tag_2 - tag_1).count();
    std::cout << "result: " << C_buf[0] << "," << C_buf[m*n-1] << std::endl;

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
