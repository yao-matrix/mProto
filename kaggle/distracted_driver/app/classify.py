#!/usr/bin/env python
# coding=utf-8

import os
import cv2
import logging
import glob
import math
import datetime
import numpy as np

import pandas as pd

from keras.models import model_from_json
from keras.models import Sequential

from ml_utils import read_model

current_dir = os.path.dirname(os.path.abspath(__file__))
log_path = os.path.join(current_dir, os.path.pardir, 'log', datetime.date.today().strftime('%Y%m%d') + '.log')

logger = logging.getLogger('classify')
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

def load_img(img_path, img_rows, img_cols):
    # read image to a grayscale buffer
    # print img_path
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    rows, cols = img.shape
    # print rows, cols

    # print img_rows, img_cols

    resized = cv2.resize(img, (img_cols, img_rows), interpolation = cv2.INTER_CUBIC);
    return resized

def load_test():
  logger.info('read test images')

  path = os.path.join('/workshop2/data/driver-distraction', 'test', '*.jpg')
  files = glob.glob(path)
  X_test = []
  X_test_id = []
  total = 0
  thr = math.floor(len(files) / 10)

  for fl in files:
    flbase = os.path.basename(fl)
    img = load_img(fl, img_rows, img_cols)
    X_test.append(img)
    X_test_id.append(flbase)
    total += 1
    if total % thr == 0:
      logger.info('read {} images from {}'.format(total, len(files)))

  return X_test, X_test_id

def create_submission(predictions, test_id):
  result = pd.DataFrame(predictions, columns = ['c0', 'c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8', 'c9'])
  result.loc['img', :] = pd.Series(test_id, index = result.index)
  now = datetime.datetime.now()
  if not os.path.isdir('submission'):
    os.mkdir('submission')
  suffix = str(now.strftime("%Y-%m-%d-%H-%M"))
  sub_file = os.path.join('submission', 'submission_' + suffix + '.csv')
  result.to_csv(sub_file, index = False)
  
if __name__ == "__main__":
    img_rows = 96
    img_cols = 128
    # load test data
    test_data, test_id = load_test()
    test_data = np.array(test_data, dtype = np.uint8)
    test_data = test_data.reshape(test_data.shape[0], 1, img_rows, img_cols)
    test_data = test_data.astype('float32')
    test_data /= 127.5
    test_data -= 1
    # load model
    model = read_model()

    predictions = model.predict(test_data, batch_size = 128, verbose = 1)
    create_submission(predictions, test_id)
