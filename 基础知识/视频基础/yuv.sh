#基础命令，生成yuv文件
ffmpeg -y -i v.mp4 \
    -an -c:v rawvideo \
    -pix_fmt yuv420p \
    -s 640x480 \
    v.yuv

ffplay  -pixel_format yuv420p -video_size 640x480  v.yuv


#基础命令，yuv422p,生成yuv文件
ffmpeg -i v.mp4 \
    -an -c:v rawvideo \
    -pix_fmt yuv422p \
    -s 320x240 v.yuv
ffplay  -pixel_format yuv422p -video_size 320x240  v.yuv


#使用简单视频过滤器，播放Y

ffplay  -pixel_format yuv420p -video_size 640x480  -vf extractplanes='y' v.yuv
ffplay  -pixel_format yuv420p -video_size 640x480  -vf extractplanes='u' v.yuv
ffplay  -pixel_format yuv420p -video_size 640x480  -vf extractplanes='v' v.yuv



#ffmpeg 复杂filter 分别提取y,u,v分量
ffmpeg -y -i v.mp4 \
    -an -c:v rawvideo \
    -pix_fmt yuv420p \
    -filter_complex 'extractplanes=y+u+v[y][u][v]' \
    -s 640x480 \
    -map '[y]' Y.yuv \
    -map '[u]' U.yuv \
    -map '[v]' V.yuv
# extractplanes=表示 提取的平面
# y+u+v 指定屏幕
# [y][u][v] 给上面三个屏幕的别名
# -map '[y]' Vy.yuv  [y]输出到Vy.yuv

#以下命令播放有问题，为什么？ 
ffplay  -pixel_format gray -video_size 640x480 Y.yuv
ffplay  -pixel_format gray -video_size 320x240 U.yuv
ffplay  -pixel_format gray -video_size 320x240 V.yuv