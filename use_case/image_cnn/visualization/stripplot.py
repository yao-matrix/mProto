#!/usr/bin/env python
# coding=utf-8

import os
import logging
import datetime

import pandas as pd
import seaborn as sns

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
  # load classification result
  data = pd.read_csv("real.csv", index_col = False)

  level = 0.0
  for i in range(10):
    col = 'c' + str(i)
    data[col] += level
    level += 1.0

  # print(data)

  sns.set_style("whitegrid")
  # sns.boxplot([data['c0'], data['class']])
  # sns.stripplot(x = 'class', y = 'c0', data = data, jitter = True)
  sns.stripplot(x = data['c0'])
