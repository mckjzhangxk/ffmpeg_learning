import cv2
'''


videoio/include/opencv2/videoio.hpp
'''


name='video'
outfile='data/record.mp4'


#设置编码器,参考 https://fourcc.org/codecs.php
fourcc=cv2.VideoWriter_fourcc('x','2','6','4')
# 原型：CV_WRAP VideoWriter(const String& filename, int fourcc, double fps,Size frameSize, bool isColor = true);
# size必须和摄像头分辨率一致
writer=cv2.VideoWriter(outfile,fourcc,25,(1920,1080))



########################老代码####################################
name='record'
cv2.namedWindow(name,cv2.WINDOW_AUTOSIZE)
# 第二个参数表示使用的api，比如direct show,vl4,ffmpeg,gstreamer

filename=0
# filename='/Users/zhanggxk/ana/jojo.mp4'
cap = cv2.VideoCapture(filename,cv2.CAP_ANY)
while True:

    ret,I=cap.read()
    if ret:
        cv2.imshow(name,I)
    key=cv2.waitKey(50)
    if key==ord('q'):
        break

    writer.write(I)
############################################################

writer.release()
cap.release()
cv2.destroyAllWindows()