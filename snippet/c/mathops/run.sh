# cpupower frequency-set -u 2.4GHZ
# cpupower frequency-set -d 2.4GHZ
# cpupower frequency-set -g performance

# cpupower idle-info
# cpupower idle-set

# scl enable devtoolset-7 bash

## vanilla
taskset -c 2 ./vec_avx

## vtune
# source /opt/intel/vtune_amplifier/amplxe-vars.sh
# amplxe-cl -collect uarch-exploration  -- ./vec_avx
# amplxe-cl -collect uarch-exploration -knob sampling-interval=1 -- taskset -c 2 ./vec_avx
# amplxe-cl -collect uarch-exploration -start-paused -resume-after 10 -knob sampling-interval=1 -- taskset -c 2 ./vec_avx
# amplxe-cl -collect memory-access -knob sampling-interval=1 -- ./vex_avx

## valgrind
# valgrind --tool=cachegrind taskset -c 1 ./vec_avx

## perf
# perf stat -e L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores,dTLB-prefetch-misses taskset -c 1 ./vec_avx
# perf stat -e cache-references,cache-misses,l1d.replacement taskset -c 1 ./vec-avx
# perf report -v

## pcm
# pcm-memory.x -- taskset -c 2 ./vec_avx
# pcm.x -- taskset -c 2 ./vec_avx
