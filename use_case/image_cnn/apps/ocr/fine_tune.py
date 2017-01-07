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

import numpy as np
from six.moves import urllib
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf

from data_io import load_imageset, split_cv
from metrics import precision, error_rate


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

SEED = 66478  # Set to None for random seed.
BATCH_SIZE = 64
NUM_EPOCHS = 200
EVAL_BATCH_SIZE = 64
EVAL_FREQUENCY = 100  # Number of steps between evaluations.


FLAGS = tf.app.flags.FLAGS

def main(argv = None):  # pylint: disable=unused-argument
  # load imageset
  train_set_folder = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'data/test')
  test_set_folder = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'data/test')

  # Extract it into numpy arrays.
  train_data, train_labels = load_imageset(train_set_folder, to_img_size = (28, 28, 1), ext = 'png')
  test_data, test_labels = load_imageset(test_set_folder, to_img_size = (28, 28, 1), ext = 'png')

  height = train_data.shape[1]
  width = train_data.shape[2]
  channel = (train_data.shape[3] if train_data.ndim > 3 else 1)

  label_max = np.amax(train_labels)
  label_min = np.amin(train_labels)
  num_labels = label_max - label_min + 1

  # Generate a validation set.
  train_data, train_labels, validation_data, validation_labels = split_cv(train_data, train_labels, 0.1)

  num_epochs = NUM_EPOCHS
  train_size = train_labels.shape[0]

  # This is where training samples and labels are fed to the graph.
  # These placeholder nodes will be fed a batch of training data at each
  # training step using the {feed_dict} argument to the Run() call below.
  train_data_node = tf.placeholder(
      tf.float32,
      shape = (BATCH_SIZE, height, width, channel))
  train_labels_node = tf.placeholder(tf.int64, shape = (BATCH_SIZE,))

  eval_data = tf.placeholder(
      tf.float32,
      shape=(EVAL_BATCH_SIZE, height, width, channel))

  # The variables below hold all the trainable weights. They are passed an
  # initial value which will be assigned when we call:
  # {tf.initialize_all_variables().run()}
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

  # We will replicate the model structure for the training subgraph, as well
  # as the evaluation subgraphs, while sharing the trainable parameters.
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

  # Training computation: logits + cross-entropy loss.
  logits = lenet2(train_data_node, True)
  loss = tf.reduce_mean(tf.nn.sparse_softmax_cross_entropy_with_logits(
      logits, train_labels_node))

  # L2 regularization for the fully connected parameters.
  regularizers = (tf.nn.l2_loss(fc1_weights) + tf.nn.l2_loss(fc1_biases) +
                  tf.nn.l2_loss(fc2_weights) + tf.nn.l2_loss(fc2_biases))
  # Add the regularization term to the loss.
  loss += 5e-4 * regularizers

  # Optimizer: set up a variable that's incremented once per batch and
  # controls the learning rate decay.
  batch = tf.Variable(0)
  # Decay once per epoch, using an exponential schedule starting at 0.01.
  learning_rate = tf.train.exponential_decay(
      0.01,                # Base learning rate.
      batch * BATCH_SIZE,  # Current index into the dataset.
      train_size,          # Decay step.
      0.95,                # Decay rate.
      staircase=True)
  # Use simple momentum for the optimization.
  optimizer = tf.train.MomentumOptimizer(learning_rate,
                                         0.9).minimize(loss,
                                                       global_step = batch)

  # Predictions for the current training minibatch.
  train_prediction = tf.nn.softmax(logits)

  # Predictions for the test and validation, which we'll compute less often.
  eval_prediction = tf.nn.softmax(lenet2(eval_data))

  # Small utility function to evaluate a dataset by feeding batches of data to
  # {eval_data} and pulling the results from {eval_predictions}.
  # Saves memory and enables this to run on smaller GPUs.
  def eval_in_batches(data, sess):
    """Get all predictions for a dataset by running it in small batches."""
    size = data.shape[0]
    if size < EVAL_BATCH_SIZE:
      raise ValueError("batch size for evals larger than dataset: %d" % size)
    predictions = np.ndarray(shape = (size, num_labels), dtype = np.float32)
    for begin in xrange(0, size, EVAL_BATCH_SIZE):
      end = begin + EVAL_BATCH_SIZE
      if end <= size:
        predictions[begin:end, :] = sess.run(
            eval_prediction,
            feed_dict={eval_data: data[begin:end, ...]})
      else:
        batch_predictions = sess.run(
            eval_prediction,
            feed_dict={eval_data: data[-EVAL_BATCH_SIZE:, ...]})
        predictions[begin:, :] = batch_predictions[begin - size:, :]
    return predictions

  # Create a local session to run the training.
  start_time = time.time()
  model_dir = os.path.join(module_dir, os.path.pardir, os.path.pardir, 'models') 
  with tf.Session() as sess:
    # Run all the initializers to prepare the trainable parameters.
    tf.initialize_all_variables().run()
    # Import base model weights
    saver = tf.train.Saver([conv1_weights, conv1_biases, conv2_weights, conv2_biases, fc1_weights, fc1_biases])
    ckpt = tf.train.get_checkpoint_state(model_dir)
    if ckpt and ckpt.model_checkpoint_path:
      logger.info("Continue training from the model {}".format(ckpt.model_checkpoint_path))
      saver.restore(sess, ckpt.model_checkpoint_path)
    # for var in tf.trainable_variables():
    #  logger.info(var.eval())


    logger.info('Initialized!')
    # Loop through training steps.
    for step in xrange(int(num_epochs * train_size) // BATCH_SIZE):
      # Compute the offset of the current minibatch in the data.
      # Note that we could use better randomization across epochs.
      offset = (step * BATCH_SIZE) % (train_size - BATCH_SIZE)
      batch_data = train_data[offset:(offset + BATCH_SIZE), ...]
      batch_labels = train_labels[offset:(offset + BATCH_SIZE)]
      # This dictionary maps the batch data (as a numpy array) to the
      # node in the graph it should be fed to.
      feed_dict = {train_data_node: batch_data,
                   train_labels_node: batch_labels}
      # Run the graph and fetch some of the nodes.
      _, l, lr, predictions = sess.run(
          [optimizer, loss, learning_rate, train_prediction],
          feed_dict=feed_dict)
      if step % EVAL_FREQUENCY == 0:
        elapsed_time = time.time() - start_time
        start_time = time.time()
        logger.info('Step %d (epoch %.2f), %.1f ms' %
              (step, float(step) * BATCH_SIZE / train_size,
               1000 * elapsed_time / EVAL_FREQUENCY))
        logger.info('Minibatch loss: %.3f, learning rate: %.6f' % (l, lr))
        logger.info('Minibatch error: %.1f%%' % error_rate(predictions, batch_labels))
        logger.info('Validation error: %.1f%%' % error_rate(
            eval_in_batches(validation_data, sess), validation_labels))
        sys.stdout.flush()
    # Finally print the result!
    test_precision = precision(eval_in_batches(test_data, sess), test_labels)
    logger.info('Test precision: %.1f%%' % test_precision)

    # Model persistence
    saver = tf.train.Saver([conv1_weights, conv1_biases, conv2_weights, conv2_biases, fc1_weights, fc1_biases])
    model_path = os.path.join(model_dir, "lenet_finetuned.ckpt")
    save_path = saver.save(sess, model_path)
    logger.info("Model saved in file: %s" % save_path)

if __name__ == '__main__':
  tf.app.run()
