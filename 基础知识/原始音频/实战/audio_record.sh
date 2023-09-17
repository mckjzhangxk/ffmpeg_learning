#这里指定了
#  对于设备 索引编号： videoId:audioId
# 输入： 设备是0
# 输出： wav格式
# 注意：mac上可能需要权限认证
# avfoundation是mac使用的驱动
ffmpeg  -f avfoundation   -i :0 out.wav


# 播放
ffplay  -s out.wav