[视频](https://www.youtube.com/watch?v=am_0vlQpJgE) [资料](https://rmcore.com/introduction-to-gstreamer/)
gstream基础语法

* property 属于element, 格式是key=value,空格隔开
```
 gst-launch-1.0  udpsrc name=u port=5004
```
* capibility  格式是key=value,逗号隔开， 表示对输出的 过滤
```
    gst-launch-1.0  videotestsrc ! "video/x-raw,width=800,height=600,framerate=30/1" !
```

* 定义多条pipeline
```
gst-launch-1.0 \
    videotestsrc name=v  v.src  \
    ! autovideosink  videotestsrc name=u u.src \ #第一条pipeline结束，创建第二个pipeline
    ! autovideosink
```
* queue 用于开启一个新的线程
```
    gst-launch-1.0 \
    videotestsrc name=v  v.src  \
    ! queue \
    ! autovideosink  videotestsrc name=u u.src \
    ! queue \
    ! autovideosink
```
* 检查文件格式
```
gst-discoverer-1.0 mit_av.mkv
```
### 实例总结
**source,sink**
```
# sender:
gst-launch-1.0 filesrc location='big_buck_bunny_1080p_stereo.ogg' ! udpsink host=127.0.0.1 port=5004
# receive
gst-launch-1.0 udpsrc ! fakesink dump=1
```
**视频转码1**
```
# 分辨率=>(320,200),帧率=>25,封装=>matroska
gst-launch-1.0 filesrc location='mit27_h264.mp4' \
    ! qtdemux  name=q q.video_0 \
    ! h264parse \
    ! avdec_h264 \
    ! videoscale method=0 ! 'video/x-raw, width=300, height=200,frame=25/1' \
    ! vtenc_h264 \  #特替换为openh264enc
    ! matroskamux \
    ! filesink location=mit_video.mkv

# qtdemux 的src 有video,audio,用video_0进行引用
# videoscale  可以用于原始 图片的处理
```
**视频转码2**
```
#h264 转vp8
gst-launch-1.0 filesrc location='mit27_h264.mp4' \
    ! qtdemux \
    ! h264parse \
    ! avdec_h264 \
    ! vp8enc \
    ! matroskamux \
    ! filesink location=mit_video.mkv
```

**音频转码**
```
# acc=>vorbis
gst-launch-1.0 filesrc location='mit27_h264.mp4' \
    ! qtdemux  name=q q.audio_0 \
    ! aacparse \
    ! avdec_aac \
    ! audioconvert \
    ! vorbisenc \
    ! matroskamux \
    ! filesink location=mit_audio.mkv
```

** HLS **
```
#mpegtsmux封装,hlssink
gst-launch-1.0 filesrc location='/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4' \
    ! qtdemux  name=q q.video_0 \
    ! queue \
    ! h264parse \
    ! avdec_h264 \
    ! videoscale method=0 ! 'video/x-raw, width=300, height=200,frame=25/1' \
    ! openh264enc \   #视频编码openh264enc
    ! mux. q.audio_0 \
    ! queue \
    ! aacparse \
    ! avdec_aac \
    ! audioconvert \
    ! avenc_aac \   #音频必须AAC
    ! mpegtsmux name=mux \
    ! hlssink
python3 -m http.server
```