#!/usr/bin/env python
# coding=utf-8

import os
import logging
import datetime

from keras.utils.visualize_util import plot

from model_io import read_model

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
  
if __name__ == "__main__":
    # load model
    model = read_keras_model(os.path.join(module_dir, os.path.pardir, 'models', 'distracted_driver'))

    plot(model, to_file = 'model.png')
