import cv2

def update_frame(mat):
    cv2.imshow('Streaming',mat)
    cv2.waitKey(1)
    return 1;

cv2.startWindowThread()
