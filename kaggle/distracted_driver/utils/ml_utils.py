#!/usr/bin/env python
# coding=utf-8

import sys
import os
import cv2

from keras.models import Sequential
from keras.models import model_from_json

from sklearn.cross_validation import train_test_split


current_dir = os.path.dirname(os.path.abspath(__file__))
model_dir = os.path.join(current_dir, os.path.pardir, 'model')

def load_img(img_path, img_rows, img_cols):
    # read image to a grayscale buffer
    print img_path
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    rows, cols = img.shape
    # print rows, cols

    # print img_rows, img_cols

    resized = cv2.resize(img, (img_cols, img_rows), interpolation = cv2.INTER_CUBIC);
    return resized


def split_cv(data, labels):
    random_state = 10
    train, validation, train_label, validation_label = train_test_split(data, labels, test_size = labels.shape[0] / 10, random_state = random_state)
    return train, train_label, validation, validation_label


def save_model(model):
    json_string = model.to_json()
    open(os.path.join(model_dir, 'arch.json'), 'w').write(json_string)
    model.save_weights(os.path.join(model_dir, 'weight.h5'), overwrite = True)


def read_model():
    model = model_from_json(open(os.path.join(model_dir, 'arch.json'), 'r').read())
    model.load_weights(os.path.join(model_dir, 'weight.h5'))
    model.compile(loss = 'categorical_crossentropy', optimizer = 'adadelta')
    return model
