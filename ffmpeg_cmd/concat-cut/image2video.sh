# 输出
# -r :帧率，每秒钟产生多少 张图片？
ffmpeg -i data/1.ts -r 20 -f image2 'data/image-%3d.jpg'



# 图片转视频,乳鸽指定帧率呢？
ffmpeg -i 'data/images/image-%3d.jpg'  my.mp4