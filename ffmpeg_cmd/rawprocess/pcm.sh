# pcm 是声音的原始文件格式

#输出：
#ar:采样率
#ac2:channel=2
#-f:输出格式s16p 表示 有符号数字,16位数，见 ffmpeg -sample_fmts??
ffmpeg -i input.mp4 -vn -ar 44100 -ac2 -f s16le output.pcm


#  一定要指定对参数，不然很 葬 耳朵
ffplay -ar 44100 -f s16le -ac 2 output.pcm


