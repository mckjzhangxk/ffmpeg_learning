PEER_IP=127.0.0.1
PEER_V=5004

gst-launch-1.0 \
    avfvideosrc \
    ! videoconvert \
    ! x264enc \
    ! rtph264pay \
    !  "application/x-rtp,payload=(int)108,clock-rate=(int)90000" \
    ! udpsink host=$PEER_IP port=$PEER_V