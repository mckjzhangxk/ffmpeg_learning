import cv2
import numpy as np;
'''

highgui/include/opencv2/highgui.hpp
'''



name="trackbar"

cv2.namedWindow(name,cv2.WINDOW_NORMAL)

cv2.createTrackbar("mybar",name,0,255,lambda x:x)



while True:
    v = cv2.getTrackbarPos("mybar", name)
    I=np.zeros((300,300,3),np.uint8)+v
    cv2.imshow(name,I)
    key=cv2.waitKey(50)
    if key==ord('q'):
        break
cv2.destroyAllWindows()