#这里指定了
# 输入： 图片的格式是 uyvy422,帧率是 30,设备是2，之前没有指定-r，所以 导致播放过快
# 输出： 是原始文件,尺寸是640x480

# 注意：mac上可能需要权限认证
ffmpeg  -f avfoundation  -r 30 -pix_fmt uyvy422 -i 2 -s 640x480 out.yuv



# 播放:需要知道分辨率，图片格式
ffplay  -s 640x480 -pix_fmt uyvy422 out.yuv