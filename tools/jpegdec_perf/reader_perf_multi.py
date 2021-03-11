import cv2
import os
from turbojpeg import TurboJPEG, TJPF_GRAY, TJSAMP_GRAY, TJFLAG_PROGRESSIVE
import time
import threading

# specifying library path explicitly
# jpeg = TurboJPEG(r'D:\turbojpeg.dll')
# jpeg = TurboJPEG('/usr/lib64/libturbojpeg.so')
# jpeg = TurboJPEG('/usr/local/lib/libturbojpeg.dylib')

# using default library installation

def decode():
    jpeg = TurboJPEG()
    image_folder = '/home/matrix/data/val/'
    cnt = 0
    time_sum = 0.0
    for fname in sorted(os.listdir(image_folder)):
        fpath = os.path.join(image_folder, fname)
        # print(fpath)
        in_file = open(fpath, 'rb')
        jpg = in_file.read()
        cnt += 1
        # (width, height, jpeg_subsample, jpeg_colorspace) = jpeg.decode_header(jpg)
        # print(width, height, jpeg_subsample, jpeg_colorspace)
        begin = time.time() * 1000
        raw = jpeg.decode(jpg)
        end = time.time() * 1000
        time_sum += end - begin
        in_file.close() 
    print("image cnt: ", cnt)
    print("time per image is(ms):", time_sum / cnt)



for i in range(52):
    print('thread %s is running...' % threading.current_thread().name)
    t = threading.Thread(target=decode, name='DecodeThread')
    t.start()
    # t.join()
    print('thread %s ended.' % threading.current_thread().name)
