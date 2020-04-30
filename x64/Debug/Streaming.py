import cv2, sys
import evaltest

sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\python36.zip')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\DLLs')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\win32\\lib')
sys.path.append('C:\\Users\\U0611205\\.conda\\envs\\pytorch\\lib\\site-packages\\Pythonwin')

print(sys.path)

#def update_frame(mat):
#    cv2.imshow('Streaming',mat)
#    cv2.waitKey(1)
#    return 1;

#cv2.startWindowThread()

def save(mat):
    evalimage(mat, "c:\output.png")
