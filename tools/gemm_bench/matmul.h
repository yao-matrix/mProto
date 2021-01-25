

template <typename EngineType,
          DataType inDtype,
          DataType outDtype>
int matmul(const char *transa, const char *transb,
           const size_t M, const size_t N, const size_t K, const float alpha,
           const void *A, const size_t lda, const void *B, const size_t ldb,
           const float beta, void *C, const size_t ldc,
           const void *bias = nullptr, bool activation = )

