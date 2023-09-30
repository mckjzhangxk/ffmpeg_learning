
# 拉流
ffmpeg -i rtmp://58.200.131.2:1935/livetv/cctv2 -c copy my.mp4


# re表示按照音视频播放的频率 来解码
# -c copy是拷贝码率
# -f flv数表示输出是flv封装，  因为输出url没有后缀，如果要改封装，需要指定-f选项！
ffmpeg -re -i demo.mp4 -f flv -c copy rtmp://58.200.131.2:1935/livetv/cctv2 
