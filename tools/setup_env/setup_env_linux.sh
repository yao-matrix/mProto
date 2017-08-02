#!/bin/bash

# check which rls
if [ -f /etc/centos-release ]; then
    OS=CentOS
elif [ -r /etc/os-release ]; then
    grep 'NAME="Ubuntu"' /etc/os-release > /dev/null 2>&1
    if [ $? == 0 ]; then
        OS=Ubuntu
    fi
else
    OS="Unknown Linux"
fi

echo "OS: "${OS}

# set pip source
if [! -d "~/.pip"]; then
    sudo mkdir ~/.pip
fi
cp pip.conf ~/.pip/

if [ ${OS} = 'Ubuntu' ]; then
    sudo apt-get update

    # vim
    sudo apt-get install -y vim

    # R
    sudo apt-get install r-base r-base-dev
    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-key E084DAB9
    sudo sh -c "echo 'deb http://cran.r-project.org/bin/linux/ubuntu precise/' >> /etc/apt/sources.list"
    sudo apt-get update
    sudo apt-get install r-base r-base-dev

    # version control
    sudo apt install -y subversion git

    # Android
    sudo apt-get install -y openjdk-8-jdk
    sudo apt-get install -y bison g++-multilib build-essential gperf libxml2-utils
    sudo apt-get install -y make cmake cmake3

    # Caffe environment
    sudo apt-get install -y libleveldb-dev liblmdb-dev libhdf5-serial-dev
    sudo apt-get install -y libsnappy-dev
    sudo apt-get install -y libopencv-dev
    sudo apt-get install -y libprotobuf-dev protobuf-compiler
    sudo apt-get install -y --no-install-recommends libboost-all-dev
    sudo apt-get install -y libgflags-dev libgoogle-glog-dev libgtest-dev
    sudo apt-get install -y libatlas-base-dev

    # pushd /usr/src/gtest
    # cmake .
    # make
    # sudo cp *a /usr/lib
    # popd

    # python packages
    sudo apt-get install -y python-dev python-pip libpython-dev
    sudo apt-get install -y python-h5py python-protobuf python-leveldb python-networkx python-nose
    sudo apt-get install -y swig doxygen

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
elif [ ${OS} = 'CentOS' ]; then
    # Caffe environment
    yum install -y python-pip
    yum install -y protobuf-devel leveldb-devel snappy-devel opencv-devel hdf5-devel boost-devel gflags-devel glog-devel lmdb-devel
fi

pip install 'sphinx>=1.4.0'
pip install sphinx_rtd_theme breathe recommonmark
pip install numpy opencv-python tqdm pandas scipy scikit-learn scikit-image matplotlib
