gst-launch-1.0  \
    audiotestsrc wave=3 freq=120 \
    ! audio/x-raw,format=U8 ! autoaudiosink