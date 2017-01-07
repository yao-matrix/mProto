#!/usr/bin/env python
# coding=utf-8

# Author: YAO Matrix (yaoweifeng0301@126.com)

import numpy as np

def precision(predictions, labels):
  """Return the precision based on dense predictions and sparse labels."""
  return (100.0 *
      np.sum(np.argmax(predictions, 1) == labels) /
      predictions.shape[0])

def error_rate(predictions, labels):
  """Return the error rate based on dense predictions and sparse labels."""
  return 100.0 - (100.0 *
      np.sum(np.argmax(predictions, 1) == labels) /
      predictions.shape[0])
