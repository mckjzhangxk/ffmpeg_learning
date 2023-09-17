
# 查询封装格式 是否支持 webm 或者 mp4 或者avi
ffmpeg -v quiet -formats|grep -E 'webm|mp4|avi'
