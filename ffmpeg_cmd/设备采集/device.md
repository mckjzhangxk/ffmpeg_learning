```sh
# 查看支持的设备
#D-输入，E 输出
ffmpeg -hide_banner -devices
```



## FrameBuffer
- 早期的Linux上的图像设备
```sh
#查看支持的参数
ffmpeg -h demuxer=fbdev

#录制命令行界面，但是无法显示
ffmpeg -f fbdev -framerate 25 -i /dev/fb0  fb0.mp4
```

## V4L2（video for linux 2)

```sh
#查看支持的参数
ffmpeg -h demuxer=v4l2

#显示设备/dev/video0支持的 pix_fmt,size属性
#设备可能会支持 h264这样的压缩格式
ffmpeg -hide_banner -f v4l2 -list_formats all -i /dev/video0

#录制摄像头
ffmpeg  -f v4l2  -i /dev/video0 -s 640x480 -pix_fmt yuv420p a.mp4
```


## avfoundation(mac)
```sh
ffmpeg -hide_banner -h demuxer=avfoundation

#列举出本机的所有可捕获的 video/audio设备
ffmpeg -f avfoundation -list_devices true -i ""
```