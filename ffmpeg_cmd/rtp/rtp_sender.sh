#1.re 表示用input.mp4 的帧率 来输出
#2.rtp不允许 有2个 stream,所以这里使用 -an
#3. 要输出sdp文件，因为rtp.payloadtype是用sdp协商的
#4.output_url是 一个  vlc服务器地址

input_file=$1
outupt_url=$2

ffmpeg  \
-re  \
-fflags +genpts \
-i $input_file  \
-an \
-c:v copy \
-payload_type 103 \
-ssrc 112233 \
-f rtp \
-sdp_file video.sdp $outupt_url

# 重要的2个参数
# -payload_type 108 \
# -ssrc 99999


# "rtp://192.168.0.100:15004?rtcpport=54321"
# -vn \
# -c:a copy \



