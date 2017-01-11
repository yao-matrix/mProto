#!/usr/bin/env python
# coding=utf-8

# Author: YAO Matrix (yaoweifeng0301@126.com)

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import gzip
import os
import sys
import time
import datetime
import logging
import glob

import cv2

import numpy as np
from six.moves import urllib
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf


module_dir = os.path.dirname(os.path.abspath(__file__))
module_name = os.path.basename(__file__).split('.')[0]

log_path = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'logs', module_name + '_' + datetime.date.today().strftime('%Y%m%d') + '.log')

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

IMAGE_HEIGHT = 28
IMAGE_WIDTH = 28
IMAGE_CHANNLE = 1
PIXEL_DEPTH = 256
NUM_LABELS = 2

SEED = 100 

FLAGS = tf.app.flags.FLAGS

def main(argv = None):  # pylint: disable=unused-argument
  # test data directory
  data_folder = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'data/ocr/test/c0')

  height = IMAGE_HEIGHT
  width = IMAGE_WIDTH
  channel = IMAGE_CHANNLE
  num_labels = NUM_LABELS

  data_node = tf.placeholder(
      tf.float32,
      shape=(1, height, width, channel))

  # The variables below hold all the trainable weights.
  conv1_weights = tf.Variable(
      tf.truncated_normal([5, 5, channel, 32],  # 5x5 filter, depth 32.
                          stddev = 0.1,
                          seed = SEED),
      name="conv1_weights")
  conv1_biases = tf.Variable(tf.zeros([32]), name = "conv1_biases")
  
  conv2_weights = tf.Variable(
      tf.truncated_normal([5, 5, 32, 64],
                          stddev = 0.1,
                          seed = SEED),
      name="conv2_weights")
  conv2_biases = tf.Variable(tf.constant(0.1, shape = [64]), name = "conv2_biases")
  
  fc1_weights = tf.Variable(  # fully connected, depth 512.
      tf.truncated_normal(
          [height // 4 * width // 4 * 64, 512],
          stddev = 0.1,
          seed = SEED),
      name = "fc1_weights")
  fc1_biases = tf.Variable(tf.constant(0.1, shape = [512]), name = "fc1_biases")
  
  fc2_weights = tf.Variable(
      tf.truncated_normal([512, num_labels],
                          stddev = 0.1,
                          seed = SEED),
      name = "fc2_weights")
  fc2_biases = tf.Variable(tf.constant(0.1, shape = [num_labels]), name = "fc2_biases")

  # net topology
  def lenet2(data, train = False):
    """LeNet2 definition."""
    # 2D convolution, with 'SAME' padding (i.e. the output feature map has
    # the same size as the input). Note that {strides} is a 4D array whose
    # shape matches the data layout: [n, h, w, c].
    conv1 = tf.nn.conv2d(data,
                         conv1_weights,
                         strides = [1, 1, 1, 1],
                         padding = 'SAME')
    # Bias and rectified linear non-linearity.
    relu1 = tf.nn.relu(tf.nn.bias_add(conv1, conv1_biases))
    # Max pooling. The kernel size spec {ksize} also follows the layout of
    # the data. Here we have a pooling window of 2, and a stride of 2.
    pool1 = tf.nn.max_pool(relu1,
                           ksize = [1, 2, 2, 1],
                           strides = [1, 2, 2, 1],
                           padding = 'SAME')
    conv2 = tf.nn.conv2d(pool1,
                         conv2_weights,
                         strides = [1, 1, 1, 1],
                         padding = 'SAME')
    relu2 = tf.nn.relu(tf.nn.bias_add(conv2, conv2_biases))
    pool2 = tf.nn.max_pool(relu2,
                           ksize = [1, 2, 2, 1],
                           strides = [1, 2, 2, 1],
                           padding = 'SAME')
    # Reshape the feature map cuboid into a 2D matrix to feed it to the
    # fully connected layers.
    pool_shape = pool2.get_shape().as_list()
    reshape = tf.reshape(pool2,
                         [pool_shape[0], pool_shape[1] * pool_shape[2] * pool_shape[3]])
    # Fully connected layer. Note that the '+' operation automatically
    # broadcasts the biases.
    fc1 = tf.nn.relu(tf.matmul(reshape, fc1_weights) + fc1_biases)
    # Add a 50% dropout during training only. Dropout also scales
    # activations such that no rescaling is needed at evaluation time.
    if train:
      fc1 = tf.nn.dropout(fc1, 0.5, seed = SEED)
    return tf.matmul(fc1, fc2_weights) + fc2_biases

  # Predictions for the test and validation, which we'll compute less often.
  prediction = tf.nn.softmax(lenet2(data_node))

  # Create a local session to run the training.
  start_time = time.time()
  model_dir = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'models') 
  with tf.Session() as sess:
    # Run all the initializers to prepare the trainable parameters.
    tf.initialize_all_variables().run()
    # Import base model weights
    saver = tf.train.Saver([conv1_weights, conv1_biases, conv2_weights, conv2_biases, fc1_weights, fc1_biases, fc2_weights, fc2_biases])
    ckpt = tf.train.get_checkpoint_state(os.path.join(model_dir, 'finetuned'))
    if ckpt and ckpt.model_checkpoint_path:
      logger.info("Predict from the model {}".format(ckpt.model_checkpoint_path))
      saver.restore(sess, ckpt.model_checkpoint_path)
    # for var in tf.trainable_variables():
    #  logger.info(var.eval())

    # Loop through dataset
    for img_path in glob.glob(os.path.join(data_folder, '*.png')):
      ## read image
      # logger.info("%s" % img_path)
      flag = -1
      if channel == 1:
        flag = cv2.IMREAD_GRAYSCALE
      elif channel == 3:
        flag = cv2.IMREAD_COLOR
      else:
        assert 0

      img = cv2.imread(img_path, flag)
      imh = img.shape[0]
      imw = img.shape[1]
      if height != imh or width != imw:
        img = cv2.resize(img, (height, width), interpolation = cv2.INTER_CUBIC)

      im_data = np.array(img, dtype = np.float32)
      im_data = (im_data - (PIXEL_DEPTH / 2.0)) / PIXEL_DEPTH
      im_data = im_data.reshape(1, height, width, 1)

      feed_dict = {data_node: im_data}
      # Run the graph and fetch predictions.
      predictions = sess.run(
            prediction,
            feed_dict = feed_dict)
      
      final_label = np.argmax(predictions, 1)
      logger.error("Image %s classified %d" % (img_path, final_label))

if __name__ == '__main__':
  tf.app.run()
