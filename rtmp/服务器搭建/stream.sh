
ffmpeg -re \
                 -i big_buck_bunny.mp4 \
                 -vcodec copy \
                 -loop -1 \
                 -c:a aac \
                 -b:a 160k \
                 -ar 44100 \
                 -strict -2 \
                 -f flv rtmp://192.168.0.106/live/bbb



ffmpeg -re \
                 -i jojo.flv \
                 -vcodec copy \
                 -loop -1 \
                 -c:a aac \
                 -b:a 160k \
                 -ar 44100 \
                 -strict -2 \
                 -f flv rtmp://127.0.0.1:1935/live/jojo


ffmpeg -re -i jojo.flv -c copy -f flv rtmp://localhost/live/livestream