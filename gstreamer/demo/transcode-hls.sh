gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux  name=q q.video_0 \
! queue \
! h264parse \
! avdec_h264 \
! videoscale method=0 ! 'video/x-raw, width=300, height=200,frame=25/1' \
! openh264enc \
! mux. q.audio_0 \
! queue \
! aacparse \
! avdec_aac \
! audioconvert \
! avenc_aac \
! mpegtsmux name=mux \
! hlssink