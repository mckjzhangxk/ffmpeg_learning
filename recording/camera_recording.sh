#这里指定了
# 输入： 图片的格式是 uyvy422,帧率是 30,设备是0
# 输出： 是原始文件

# 注意：mac上可能需要权限认证
ffmpeg  -f avfoundation  -r 30 -pix_fmt uyvy422 -i 0 out.yuv

# 以下指定了size， 等价的 hd720=1280x720
# ffmpeg  -f avfoundation -s hd720 -r 30 -pix_fmt uyvy422 -i 0 out.yuv
# ffmpeg  -f avfoundation -s 1280x720 -r 30 -pix_fmt uyvy422 -i 0 out.yuv

# 播放，这个 size大小 参考摄像头录制大小
ffplay  -s 640x480 -pix_fmt uyvy422 out.yuv