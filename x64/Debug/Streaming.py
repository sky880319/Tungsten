import cv2
import numpy as np

def update_frame(mat):
    cv2.imshow('Streaming', mat)
    cv2.waitKey(1)

cv2.startWindowThread()
