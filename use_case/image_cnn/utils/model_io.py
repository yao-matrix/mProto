#!/usr/bin/env python
# coding=utf-8

# Author: YAO Matrix (yaoweifeng0301@126.com)

import sys
import os
import cv2
import gzip

import numpy as np

from glob import glob

module_dir = os.path.dirname(os.path.abspath(__file__))

def save_keras_model(to_dir):
    json_string = model.to_json()
    open(os.path.join(to_dir, 'arch.json'), 'w').write(json_string)
    model.save_weights(os.path.join(to_dir, 'weight.h5'), overwrite = True)


def read_keras_model(to_dir):
    model = model_from_json(open(os.path.join(to_dir, 'arch.json'), 'r').read())
    model.load_weights(os.path.join(to_dir, 'weight.h5'))
    model.compile(loss = 'categorical_crossentropy', optimizer = 'adadelta')
    return model
