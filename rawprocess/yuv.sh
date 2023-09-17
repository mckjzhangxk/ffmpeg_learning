#yuv是 视频的基 文件的原始 格斯

# 输出
# -c:v rawvideo 表示输出解码后数据
# -pix_fmt 输出的 图片格式 

ffmpeg -i input.mp4 -an -c:v rawvideo -pix_fmt yuv420p -s 640x480 out.yuv

# 很显然这里默认 pix_fmt=yun420p，所以我不需要指定
ffplay -s 640x480 out.yuv