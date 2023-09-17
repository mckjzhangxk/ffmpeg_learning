[教材](https://www.bilibili.com/video/BV16r4y1K7J7/?spm_id_from=333.999.0.0)

# RTSP (Real time Streaming Protocal)

- 位于RTP,SDP之上的多媒体串流应用层协议。
- - RTP用于传输，SDP用于控制
- 客户端，服务器双向通信。
- 底层协议可以是 TCP,UDP，HTTP
- RTSP是基于文本的协议。每一行以\r\n结束,负责发送控制信息（相当于SDP）。


# 流程图
![Alt text](imgs/rtsp.png)
```sh
ffplay -rtsp_transport tcp "rtsp://admin:123456@118.140.224.230:554/chID=000000010-0000-0000-0000-000000000000&streamType=main&linkType=tcpst"
```

- 第二部Describe很重要，客户端发生Describe命令，服务器的回复中带有SDP,描述的媒体的信息。