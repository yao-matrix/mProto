rm $1
# export LD_LIBRARY_PATH=/home/intel/gcc9.3.0/lib64:$LD_LIBRARY_PATH
# export PATH=/home/intel/gcc9.3.0/bin:$PATH
set -e
g++ -fPIC -m64 -Wall -g -O3 -mpopcnt -msse4 -mavx -mavx2 -mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512vbmi -mavx512vnni $1.c -o $1
taskset -c 2 ./$1
