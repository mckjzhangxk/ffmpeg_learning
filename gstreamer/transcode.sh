# https://stackoverflow.com/questions/28036830/how-to-use-gstreamer-for-transcoding-and-resizing-from-mp4h264-aac-to-mp4h264

# qtdemux 的src 有video,audio,用video_0进行引用
# videoscale  可以用于原始 图片的处理

# 视频转码 转分辨率
gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux  name=q q.video_0 \
! h264parse \
! avdec_h264 \
! videoscale method=0 ! 'video/x-raw, width=300, height=200,frame=25/1' \
! vtenc_h264 \
! matroskamux \
! filesink location=mit_video.mkv

# 视频转码2 编码格式
gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux \
! h264parse \
! avdec_h264 \
! vp8enc \
! matroskamux \
! filesink location=mit_video.mkv

# filesrc location= ! qtdemux name=q q.audio_0 ! queue ! mpegaudioparse ! mpg123audiodec ! audioconvert ! avenc_aac !  matroskamux !filesink location=bbb.mkv

# 音频转码

gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux  name=q q.audio_0 \
! aacparse \
! avdec_aac \
! audioconvert \
! vorbisenc \
! matroskamux \
! filesink location=mit_audio.mkv

# 全部转换
gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
! qtdemux  name=q q.video_0 \
! queue \
! h264parse \
! avdec_h264 \
! videoscale method=0 ! 'video/x-raw, width=300, height=200,frame=25/1' \
! vtenc_h264 \
! mux. q.audio_0 \
! queue \
! aacparse \
! avdec_aac \
! audioconvert \
! vorbisenc \
!matroskamux name=mux \
! filesink location=mit_av.mkv
