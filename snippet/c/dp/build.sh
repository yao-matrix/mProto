# export http_proxy="http://child-prc.intel.com:913"
# export https_proxy="https://child-prc.intel.com:913"

# yum install devtoolset-7
# scl enable devtoolset-7 bash

rm -rf dp dp.o
make
