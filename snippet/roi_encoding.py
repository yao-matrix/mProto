import cv2
import numpy as np


def psnr(imgA, imgB):
  if not np.array_equal(imgA.shape, imgB.shape):
    return -1;

  err = np.sum((imgA.astype("float") - imgB.astype("float")) ** 2)
  mse = err / imgA.size
  psnr = 10.0 * np.log10(255.0 * 255.0 / mse)
  return psnr


img = cv2.imread(r'test.jpg');
print img.shape

ret, jpg2 = cv2.imencode(".jpg", img, [int(cv2.IMWRITE_JPEG_QUALITY), 60])
img2 = cv2.imdecode(jpg2, 1)
print img2.shape
psnr1 = psnr(img, img2);
print psnr1

cv2.imshow('original image', img)
cv2.waitKey(0)
cv2.imshow('image', img2)
cv2.waitKey(0)
