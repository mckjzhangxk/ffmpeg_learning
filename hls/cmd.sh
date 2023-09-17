# 以下是ffempeg把一个文件转成ts的代码

# segment_list_type 列表文件的类型
# segment_time每个ts文件 的时间
# segment_list_size 每个列表文件 的条目数量
# segment_list：列表文件的 路径
# -bsf:v h264_mp4toannexb  表示H264字节流的nalu 前加入startcode

ffmpeg -re -i 039cc412fb4054b23b730f003a582c6e.avi \
       -an -c:v copy -bsf:v h264_mp4toannexb \
       -f segment \
       -segment_list_type m3u8 \
       -segment_time 3 \
       -segment_list_size 5 \
       -segment_list_flags live \
       -segment_list ./lal_record/hls/039cc412fb4054b23b730f003a582c6e/playlist.m3u8 \
       ./lal_record/hls/039cc412fb4054b23b730f003a582c6e/039cc412fb4054b23b730f003a582c6e-%10d-00.ts



ffmpeg -re -i 039cc412fb4054b23b730f003a582c6e.avi \
       -an -c:v copy -bsf:v h264_mp4toannexb \
       -segment_list_type m3u8 \
       -segment_format mp4 \
       -segment_time 5 \
       -segment_list_size 3 \
       -segment_list ./lal_record/hls/039cc412fb4054b23b730f003a582c6e/playlist.m3u8 \
       ./lal_record/hls/039cc412fb4054b23b730f003a582c6e/039cc412fb4054b23b730f003a582c6e-%5d-0.ts



ffmpeg -re -i %s.avi -an -c:v copy -bsf:v h264_mp4toannexb -f segment -segment_list_type m3u8 -segment_format mp4 -segment_time 5 -segment_list_size 3 -segment_list_flags live -segment_list %s/playlist.m3u8 %s/%s-%%5d.ts






//会产生异常的命令
ffmpeg  -i 39671f8ce7910af5d3faa5c0f073ce26-1.avi \
       -an -c:v copy  \
       -hls_time 8 \
       -hls_list_size 0 \
       -hls_segment_filename ./video1/039cc412fb4054b23b730f003a582c6e-%5d-00.ts \
       ./video1/playlist.m3u8

ffmpeg  -fflags +genpts  -i 39671f8ce7910af5d3faa5c0f073ce26-1.avi \
       -an -c:v copy  \
       -hls_time 8 \
       -hls_list_size 0 \
       -hls_segment_filename ./video1/039cc412fb4054b23b730f003a582c6e-%5d-00.ts \
       ./video1/playlist.m3u8
        




ffmpeg  -i 39671f8ce7910af5d3faa5c0f073ce26-1.avi \
       -an -c:v copy  -bsf:v h264_mp4toannexb \
       -segment_list_type m3u8 \
       -segment_format cache \
       -segment_time 8 \
       -segment_list_size 0 \
       -segment_list ./video1/playlist.m3u8 \
       -f segment \
       ./video1/39671f8ce7910af5d3faa5c0f073ce26-%5d-00.ts