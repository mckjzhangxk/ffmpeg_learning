import cv2
import numpy as np;
'''

highgui/include/opencv2/highgui.hpp
'''
# flags-组合键
# event：移动 点击
def mouse_callback ( event, x, y, flags, userdata ):
    print(event, x, y, flags, userdata)

name="mouse"

cv2.namedWindow(name,cv2.WINDOW_NORMAL)
#设置鼠标回调
cv2.setMouseCallback(name,mouse_callback,param="ss")

cv2.imshow(name,np.zeros((300,300,3)))
key=cv2.waitKey(0)
cv2.destroyAllWindows()