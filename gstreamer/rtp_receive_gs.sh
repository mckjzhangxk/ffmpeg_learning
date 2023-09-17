# 属性属于element key=value,空格隔开
# cap key=value, 逗号隔开， ! <caps语句> !,表示对输出的过滤
# name属性  表示引用， 有些【复合元素】必须使用才可以引用 【内部元素】
# element 之间可以并列的，中间的那些元素 不参与 <输入输出>
# source ! sink, 有些元素既可以是source也可以是sink,比如rtph264depay,decodebin
SELF_V=5004 \
CAPS_V="media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264,payload=(int)123" \
bash -c 'gst-launch-1.0 -e \
         udpsrc name=u address=0.0.0.0 port=$SELF_V  rtpsession name=r  u.src ! "application/x-rtp,$CAPS_V" \
        ! r.recv_rtp_sink autovideosink name=myoutput  r.recv_rtp_src ! rtph264depay ! decodebin ! myoutput.sink'
