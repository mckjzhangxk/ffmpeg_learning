
# https://ffmpeg.org/ffmpeg-codecs.html#Options-12

#编译的时候需要enable lib_fdk_aac
# brew reinstall fdk-aac
# brew reinstall opus
#profile：规格
# aac_he_v2,aac_he,aac_low
# ar采样率
# channels 声道数
ffmpeg -i my.mp4 \
     -vn \
     -c:a libfdk_aac \
     -ar 44100 \
     -channels 2 \
     -profile:a aac_he_v2 \
     out.aac

#profile=aac_low
ffmpeg -i my.mp4 \
     -vn \
     -c:a libfdk_aac \
     -ar 44100 \
     -channels 2 \
     -profile:a aac_low \
     out.aac
#opus参数比较少
ffmpeg -i my.mp4 \
     -vn \
     -c:a libopus \
     out.opus