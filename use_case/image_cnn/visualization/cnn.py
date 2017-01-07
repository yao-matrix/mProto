#!/usr/bin/env python
# coding=utf-8

import os
import logging
import datetime

from keras.utils.visualize_util import plot

from ml_utils import read_model

current_dir = os.path.dirname(os.path.abspath(__file__))
log_path = os.path.join(current_dir, os.path.pardir, 'log', datetime.date.today().strftime('%Y%m%d') + '.log')

logger = logging.getLogger('visualize')
logger.setLevel(logging.DEBUG)
fh = logging.FileHandler((log_path)
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
