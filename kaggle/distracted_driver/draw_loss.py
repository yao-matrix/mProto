#!/usr/bin/env python
# coding=utf-8

import matplotlib.pyplot as plt
import numpy as np

x = np.arange(1e-6, 1., 0.01)
y = np.log(x)

plt.plot(x, y)
plt.show()
