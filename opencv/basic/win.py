import cv2
'''

highgui/include/opencv2/highgui.hpp
imgcodecs/include/opencv2/imgcodecs.hpp

'''
name='new'
# 关注ImreadModes
I=cv2.imread('data/img.png',cv2.IMREAD_COLOR)


# WINDOW_AUTOSIZE:不能调整窗口大小
cv2.namedWindow(name,cv2.WINDOW_NORMAL)
cv2.imshow(name,I)

#不能和WINDOW_AUTOSIZE一起使用
cv2.resizeWindow(name,700,600)


key=cv2.waitKey(0)
print(f'key={key}')

cv2.destroyAllWindows()