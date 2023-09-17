# 输出:
# -ss :set start time
# -t: duration
ffmpeg -i data/33.mp4 -ss 00:00:00 -t 10 data/1.ts
ffmpeg -i data/33.mp4 -ss 00:00:10 -t 10 data/2.ts
ffmpeg -i data/33.mp4 -ss 00:00:20 -t 10 data/3.ts



# 输入
# -f concat 指定输入格式
# -i concat.txt 
ffmpeg -f concat -i concat.txt data/merge.flv