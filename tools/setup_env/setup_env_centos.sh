#!/bin/sh

# Caffe environment
yum install -y python-pip
yum install -y protobuf-devel leveldb-devel snappy-devel opencv-devel hdf5-devel boost-devel gflags-devel glog-devel lmdb-devel
pip install opencv-python tqdm pandas scipy scikit-learn scikit-image matplotlib
