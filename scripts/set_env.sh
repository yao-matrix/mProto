sudo apt-get update

# vim
sudo apt-get install vim

# version control
sudo apt install subversion git

# Android
sudo apt-get install openjdk-8-jdk
sudo apt-get install bison g++-multilib build-essential gperf libxml2-utils
sudo apt-get install make cmake

# Caffe
sudo apt-get install libleveldb-dev liblmdb-dev libhdf5-serial-dev
sudo apt-get install libsnappy-dev
sudo apt-get install libopencv-dev 
sudo apt-get install -y libprotobuf-dev protobuf-compiler
sudo apt-get install --no-install-recommends libboost-all-dev
sudo apt-get install libgflags-dev libgoogle-glog-dev libgtest-dev
sudo apt-get install libatlas-base-dev

pushd /usr/src/gtest
cmake .
make
sudo cp *a /usr/lib
popd

# python packages
sudo apt-get install python-dev python-pip libpython-dev
sudo apt-get install python-numpy python-protobuf
sudo apt-get install swig doxygen
pip install 'sphinx>=1.4.0'
pip install sphinx_rtd_theme breathe recommonmark

# project management
sudo apt-get install maven
