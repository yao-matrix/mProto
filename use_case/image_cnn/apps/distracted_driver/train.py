#!/usr/bin/env python
# coding=utf-8

import os
import cv2
import glob
import datetime
import logging
import numpy as np

from keras.utils import np_utils
from keras.models import Sequential
from keras.layers.core import Dense, Dropout, Activation, Flatten
from keras.layers.convolutional import Convolution2D, MaxPooling2D

from data_io import load_imageset, split_cv
from metrics import precision, error_rate

module_dir = os.path.dirname(os.path.abspath(__file__))
module_name = os.path.basename(__file__).split('.')[0]

log_path = os.path.join(module_dir, os.path.pardir, 'logs', module_name + '_' + datetime.date.today().strftime('%Y%m%d') + '.log')

logger = logging.getLogger(module_name)
logger.setLevel(logging.DEBUG)
fh = logging.FileHandler(log_path)
ch = logging.StreamHandler()
fh.setLevel(logging.DEBUG)
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('[%(asctime)s][%(name)s][%(levelname)s]: %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)


img_height = 96
img_width = 128

batch_size = 64
nb_classes = 10
nb_epoch = 2
nb_filters = 32
nb_pool = 2
nb_conv = 3

if __name__ == "__main__":
  logger.info("start training")

  train_set_folder = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'data/test')
  test_set_folder = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'data/test')

  # read training data
  train_data, train_labels = load_imageset(train_set_folder, to_img_size = (img_height, img_width, 1), ext = 'png')

  # split for cross validation
  train, train_label, validation, validation_label = split_cv(train_data, train_labels)
  logger.info("data split complete")

  # build stacking layers
  model = Sequential()
  model.add(Convolution2D(nb_filters, nb_conv, nb_conv, border_mode = 'valid', input_shape = (1, img_height, img_width)))
  model.add(Activation('relu'))

  model.add(Convolution2D(nb_filters, nb_conv, nb_conv))
  model.add(Activation('relu'))

  model.add(MaxPooling2D(pool_size = (nb_pool, nb_pool)))
  model.add(Dropout(0.25))

  model.add(Flatten())
  model.add(Dense(128))
  model.add(Activation('relu'))
  model.add(Dropout(0.5))
  model.add(Dense(nb_classes))
  model.add(Activation('softmax'))

  model.compile(loss = 'categorical_crossentropy', optimizer = 'adadelta')

  model.fit(train, train_label, batch_size = batch_size, nb_epoch = nb_epoch, verbose = 1, validation_data = (validation, validation_label))

  logger.info("model training complete")

  score = model.evaluate(validation, validation_label, verbose = 0)

  logger.info("validation score: %f" % (score))

  save_model(model)
  logger.info("model saved")

