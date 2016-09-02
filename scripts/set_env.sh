sudo apt-get update

# vim
sudo apt-get install -y vim

# version control
sudo apt install -y subversion git

# Android
sudo apt-get install -y openjdk-8-jdk
sudo apt-get install -y bison g++-multilib build-essential gperf libxml2-utils
sudo apt-get install -y make cmake

# Caffe
sudo apt-get install -y libleveldb-dev liblmdb-dev libhdf5-serial-dev
sudo apt-get install -y libsnappy-dev
sudo apt-get install -y libopencv-dev
sudo apt-get install -y libprotobuf-dev protobuf-compiler
sudo apt-get install -y --no-install-recommends libboost-all-dev
sudo apt-get install -y libgflags-dev libgoogle-glog-dev libgtest-dev
sudo apt-get install -y libatlas-base-dev

pushd /usr/src/gtest
cmake .
make
sudo cp *a /usr/lib
popd

# python packages
sudo apt-get install -y python-dev python-pip libpython-dev
sudo apt-get install -y python-numpy python-protobuf
sudo apt-get install -y swig doxygen
pip install 'sphinx>=1.4.0'
pip install sphinx_rtd_theme breathe recommonmark

# project management
sudo apt-get install -y maven

# ffmpeg & dependencies
sudo apt-get install -y autoconf automake libass-dev libfreetype6-dev libsdl1.2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev
sudo apt-get install -y libxcb-xfixes0-dev pkg-config texinfo zlib1g-dev
sudo apt-get install -y yasm
sudo apt-get install -y libx264-dev
sudo apt-get install -y libfdk-aac-dev
sudo apt-get install -y libmp3lame-dev
sudo apt-get install -y libopus-dev
sudo apt-get install -y ffmpeg
