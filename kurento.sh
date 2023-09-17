PEER_A=45496 PEER_V=39070 PEER_IP=60.205.223.188 \
SELF_PATH="/Users/zzzlllll/myproject/ffmpegLearning/rtmp/mit27_h264.mp4" \
SELF_A=5006 SELF_ASSRC=445566 \
SELF_V=5004 SELF_VSSRC=112233 \
bash -c 'gst-launch-1.0 -e \
    rtpbin name=r sdes="application/x-rtp-source-sdes,cname=(string)\"user\@example.com\"" \
    uridecodebin uri="file://$SELF_PATH" name=d \
    d. ! queue \
        ! audioconvert ! audioresample ! opusenc \
        ! rtpopuspay \
        ! "application/x-rtp,payload=(int)96,clock-rate=(int)48000,ssrc=(uint)$SELF_ASSRC" \
        ! r.send_rtp_sink_0 \
    d. ! queue \
        ! videoconvert ! x264enc tune=zerolatency \
        ! rtph264pay \
        ! "application/x-rtp,payload=(int)103,clock-rate=(int)90000,ssrc=(uint)$SELF_VSSRC" \
        ! r.send_rtp_sink_1 \
    r.send_rtp_src_0 \
        ! udpsink host=$PEER_IP port=$PEER_A bind-port=$SELF_A \
    r.send_rtcp_src_0 \
        ! udpsink host=$PEER_IP port=$((PEER_A+1)) bind-port=$((SELF_A+1)) sync=false async=false \
    udpsrc port=$((SELF_A+1)) \
        ! r.recv_rtcp_sink_0 \
    r.send_rtp_src_1 \
        ! udpsink host=$PEER_IP port=$PEER_V bind-port=$SELF_V \
    r.send_rtcp_src_1 \
        ! udpsink host=$PEER_IP port=$((PEER_V+1)) bind-port=$((SELF_V+1)) sync=false async=false \
    udpsrc port=$((SELF_V+1)) \
        ! tee name=t \
        t. ! queue ! r.recv_rtcp_sink_1 \
        t. ! queue ! fakesink dump=true async=false'