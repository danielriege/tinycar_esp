from configparser import Interpolation
import numpy as np
import cv2
import sys 

file = open(sys.argv[1], "rb")

data = b""

while True:
    x = file.read(1024)
    data += x
    if not x:
        break
offset = 0x01
w = 400
h = 296
cbcroff = 0
l = w * h + 2 * (w // 2 * h)
print(len(data)-offset)

img = np.frombuffer(data[offset: offset + l], dtype=np.uint8).reshape(-1, w,2 )
print(img.shape)
# Y = img[:h,:]

# CbCr = img[h + cbcroff:,:].reshape(h,w//2,2)

# cv2.imshow("Y", Y)
# cv2.imshow("Cb", CbCr[:,:,0])
# cv2.imshow("Cr", CbCr[:,:,1])

bgr = cv2.cvtColor(img, cv2.COLOR_YUV2BGR_Y422)
# bgr = cv2.cvtColor(img, cv2.COLOR_YUV2BGR_UYVY)
cv2.imshow("bgr", bgr)
cv2.waitKey(0)