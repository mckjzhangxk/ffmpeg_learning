
ffmpeg  -f avfoundation   -r 30  -pix_fmt uyvy422 -i 0:0 -s hd720 out.mp4

ffplay out.mp4