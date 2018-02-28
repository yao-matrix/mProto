# CPU BKMs
CPU sucks!

## General CPU BKMs
a. frequency check
```shell
$ turbostat -p -c 1 -i 1
```


## MKL BKMs
a. MKL log verbosity
```shell
  $ export MKL_VERBOSE=1
```

b. [Dispatch specified instruction set in MKL(like AVX2, AVX512 etc.) ](https://software.intel.com/en-us/mkl-linux-developer-guide-instruction-set-specific-dispatching-on-intel-architectures)

c. [Use MKL direct all to improve small size problem performance](
https://software.intel.com/en-us/mkl-linux-developer-guide-improving-performance-for-small-size-problems)

d. [Use packed GEMM to amortize one-time re-layout overhead](https://software.intel.com/en-us/articles/introducing-the-new-packed-apis-for-gemm)

e. [Use batch GEMM to group and compute many small size problems together](https://software.intel.com/en-us/articles/introducing-batch-gemm-operations)

f. MKL SGEMM optimization pitfalls
	1.Leading dimensions should be multiple of cache line (multiple of 16 for single precision), but not multiple of 256. Recommendation is (multiple of 128 – 16).
	2. All addresses (A, B and C) are aligned at least for cache line.
If each size is small enough (roughly less than L2 cache size) and not bad leading dimensions, no copy routine will be used.
	3. Only N-N case will performs the best. Other type of matrix needs transposing operation and performance will be similar to copy based.
	4. Even N-N case, if leading dimension for A isn’t multiple of cache line or A is not aligned for cache line, copy will be performed inside of no-copy kernel. Performance will be similar to copy based.
	5. Since element size is 16 for single precision, computation time for M=1 and M=16 are almost same. So if M is not multiple 16, performance will be automatically low. 