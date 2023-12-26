import cv2
'''

videoio/include/opencv2/videoio.hpp
'''
name='video'


cv2.namedWindow(name,cv2.WINDOW_NORMAL)
# 第二个参数表示使用的api，比如direct show,vl4,ffmpeg,gstreamer

filename=0
# filename='/Users/zhanggxk/ana/jojo.mp4'
cap = cv2.VideoCapture(filename,cv2.CAP_ANY)

#摄像头是否被打开状态
while cap.isOpened():
    ret,I=cap.read()
    if ret:
        cv2.imshow(name,I)
        cv2.resizeWindow(name,320,240)
    key=cv2.waitKey(50)
    if key==ord('q'):
        break


cap.release()
cv2.destroyAllWindows()