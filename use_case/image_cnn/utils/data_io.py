#!/usr/bin/env python
# coding=utf-8

# Author: YAO Matrix (yaoweifeng0301@126.com)

import sys
import os
import cv2
import gzip
import datetime
import logging

import numpy as np

from glob import glob

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

# Load MNIST data
def load_minst_data(t = 'train'):
  """Extract mnist images into a 4D tensor [image index, y, x, channels].
  t shoud be in {'train', test}
  Values are rescaled from [0, 255] down to [-0.5, 0.5].
  """

  data_file_name = None
  label_file_name = None
  if t == 'train':
    data_file_name = os.path.join(module_dir, os.path.pardir, 'data/mnist/train-images-idx3-ubyte.gz')
    label_file_name = os.path.join(module_dir, os.path.pardir, 'data/mnist/train-labels-idx1-ubyte.gz')
  elif t == 'test':
    data_file_name = os.path.join(module_dir, os.path.pardir, 'data/mnist/t10k-images-idx3-ubyte.gz')
    label_file_name = os.path.join(module_dir, os.path.pardir, 'data/mnist/t10k-labels-idx1-ubyte.gz')
  else:
    assert t in ['train', 'test']

  data = None
  with gzip.open(data_file_name) as bytestream:
    PIXEL_DEPTH = 256

    bytestream.read(4)
    image_number = 0
    for i in range(4):
      image_number = image_number * 256 + ord(bytestream.read(1))
    image_height = 0
    for i in range(4):
      image_height = image_height * 256 + ord(bytestream.read(1))
    image_width = 0
    for i in range(4):
      image_width = image_width * 256 + ord(bytestream.read(1))
    logger.info('image number: ' + str(image_number) + ' image height: ' + str(image_height) + ' image width: ' + str(image_width))
    buf = bytestream.read(image_height * image_width * image_number)
    data = np.frombuffer(buf, dtype = np.uint8).astype(np.float32)
    data = (data - (PIXEL_DEPTH / 2.0)) / PIXEL_DEPTH
    data = data.reshape(image_number, image_height, image_width, 1)

  labels = None
  with gzip.open(label_file_name) as bytestream:
    bytestream.read(4)
    image_number = 0
    for i in range(4):
      image_number = image_number * 256 + ord(bytestream.read(1))
    logger.info('image number: ' + str(image_number))
    buf = bytestream.read(1 * image_number)
    labels = np.frombuffer(buf, dtype = np.uint8).astype(np.int64)

  return data, labels

def load_img(img_path, to_img_size = None):
    # print img_path
    flag = -1
    if to_img_size is None or to_img_size[2] == -1:
      flag = -1
    elif to_img_size[2] == 1:
      flag = cv2.IMREAD_GRAYSCALE
    elif to_img_size[2] == 3:
      flag = cv2.IMREAD_COLOR
    else:
      assert 0

    img = cv2.imread(img_path, flag)
    height = img.shape[0]
    width = img.shape[1]
    # print img.shape

    # print to_img_size

    if to_img_size != None and (height != to_img_size[0] or width != to_img_size[1]):
      resized = cv2.resize(img, (to_img_size[1], to_img_size[0]), interpolation = cv2.INTER_CUBIC);
      return resized
    else:
      return img

def load_imageset(imageset_path, to_img_size = None, ext = 'jpg'):
  """load images under imageset_path into a 4D tensor [image index, y, x, channels] and re-size if needed.
  Values are rescaled from [0, 255] down to [-0.5, 0.5].
  """

  cat_paths = [files for files in glob(imageset_path + "/*") if os.path.isdir(files)]
  cat_paths.sort()
  cats = [os.path.basename(cat_path) for cat_path in cat_paths]
  num_labels = len(cats)  

  image_number = 0
  X = []
  Y = []
  for j in range(num_labels):
    path = os.path.join(cat_paths[j], '*.' + ext)
    # print path
    files = glob(path)
    cat = int(cats[j][1:])
    for image in files:
      image_number += 1
      # print image
      img = load_img(image, to_img_size)
      # print img.shape
      X.append(img)
      Y.append(cat)
  PIXEL_DEPTH = 256
  data = np.array(X, dtype = np.float32)
  data = (data - (PIXEL_DEPTH / 2.0)) / PIXEL_DEPTH
  data = data.reshape(image_number, data.shape[1], data.shape[2], 1)
  labels = np.array(Y, dtype = np.int64)

  logger.info("%d samples in imageset" % (image_number))
  return data, labels

def split_cv(data, labels, validation_frac):
    validation_size = int(len(data) * validation_frac)
    r = np.random.permutation(len(data))
    data = data[r, :]
    labels = labels[r]
    train_data = data[validation_size:, :]
    train_labels = labels[validation_size:]
    validation_data = data[:validation_size, :]
    validation_labels = labels[:validation_size]
    return train_data, train_labels, validation_data, validation_labels


if __name__ == "__main__":
  load_minst_data(t = 'train')
  load_minst_data(t = 'test')
  # load_minst_data(t = 'validation')

  test_path = os.path.join(module_dir, os.path.pardir, 'data/test')
  data, labels = load_imageset(test_path, to_img_size = (28, 28, 3), ext = 'png')
  # print data
  split_cv(data, labels, 0.1)
