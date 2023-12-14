播放端
```shell
ffplay -rtsp_transport tcp rtsp://10.0.4.17:7777/main

```

服务器
```shell
 ./rtsp_tcp_server    
 
# proxy的FILENAME设置为h264的插播文件
# h264nal2rtp_send的ish265设置成false
 ./proxy    
```

命令端
```shell
kill -30 pid 
kill -31 pid 

```