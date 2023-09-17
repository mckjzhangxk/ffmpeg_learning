# mac
gst-launch-1.0  \
    videotestsrc ! video/x-raw,width=800,height=600,framerate=30/1,format=UYVY ! osxvideosink

# linux
gst-launch-1.0  \
    videotestsrc ! video/x-raw,width=800,height=600,framerate=30/1,format=UYVY ! ximagesink



# silly 往udp的5004端口发送 数据， 就会看到 dump的数据

# sender:
gst-launch-1.0 filesrc location='/Users/zzzlllll/Desktop/big_buck_bunny_1080p_stereo.ogg' ! udpsink host=127.0.0.1 port=5004
# receive
gst-launch-1.0 udpsrc ! fakesink dump=1

1
#转码 
# https://stackoverflow.com/questions/28036830/how-to-use-gstreamer-for-transcoding-and-resizing-from-mp4h264-aac-to-mp4h264
gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux \
! h264parse \
! avdec_h264 \
! videoscale ! 'video/x-raw, width=800, height=600' \
! vtenc_h264 \
! matroskamux \
! filesink location=bbb.mkv