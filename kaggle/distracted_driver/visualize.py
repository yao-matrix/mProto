#!/usr/bin/env python
# coding=utf-8


import logging
import datetime

from keras.utils.visualize_util import plot

from ml_utils import read_model

logger = logging.getLogger('visualize')
logger.setLevel(logging.DEBUG)
fh = logging.FileHandler('visualize_' + datetime.date.today().strftime('%Y%m%d') + '.log')
ch = logging.StreamHandler()
fh.setLevel(logging.DEBUG)
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('[%(asctime)s][%(name)s][%(levelname)s]: %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)
  
if __name__ == "__main__":
    # load model
    model = read_model()

    plot(model, to_file = 'model.png')
