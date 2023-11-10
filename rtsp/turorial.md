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

- 第二部Describe很重要，客户端发Describe命令，服务器的回复中带有SDP,描述的媒体的信息。

# 协议解析

- 协议的每一行以\r\n结束，消息体(request/response)以\r\n结束。
- 消息的内容是 key: value的形式
- 请求头  [method] [url] rtsp/1.0
- 响应头  rtsp/1.0 [status_code] [message]
- Cseq：1 是序列号

## options:
```
# 请求(OPTIONS)
OPTIONS rtsp://118.140.224.230:554/main&linkType=tcpst 

RTSP/1.0\r\n
CSeq: 1\r\n
\r\n

#响应(Reply)
RTSP/1.0 200 OK\r\n
CSeq: 1\r\n
Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER\r\n
\r\n
```
- hello请求相当于双方的hello,客户端给服务器打招呼
- 服务器给客户端返回支持的方法

## Describe
```
# 请求(DESCRIBE)
DESCRIBE rtsp://118.140.224.230:554/main RTSP/1.0\r\n

Accept: application/sdp\r\n
CSeq: 2\r\n
\r\n

# 响应(Reply)
RTSP/1.0 401 Unauthorized\r\n
CSeq: 2\r\n
Date: Mon, Oct 30 2023 08:53:01 GMT\r\n
Expires: Mon, Oct 30 2023 08:53:01 GMT\r\n
WWW-Authenticate: Digest realm="RTSP SERVER", nonce="6bcd6f166984f72cb49fbbabe613723a"\r\n
\r\n


# 再次请求(DESCRIBE)
DESCRIBE rtsp://118.140.224.230:554/main RTSP/1.0\r\n

Accept: application/sdp\r\n
CSeq: 3\r\n

Authorization: Digest username="admin", realm="RTSP SERVER", nonce="6bcd6f166984f72cb49fbbabe613723a", uri="rtsp://118.140.224.230:554/main", response="7000e0aad46f3534db81952c43513545"\r\n

\r\n


# 响应(Reply)
RTSP/1.0 401 Unauthorized\r\n
CSeq: 3\r\n
...
...
\r\n

sdp的内容
```


```
v=0
o=- 2005515805 1 IN IP4 0.0.0.0
s=RTSP SESSION
u=http:///
e=admin@
t=0 0
a=control:*
a=range:npt=00.000- 
m=video 0 RTP/AVP 98
a=control:trackID=0
a=rtpmap:98 H265/90000
a=fmtp:62 profile-level-id=42e032; sprop-parameter-sets=RAHA8vA8kA==; packetization-mode=1



v=0
o=- 0 0 IN IP4 127.0.0.1
s=No Name
c=IN IP4 127.0.0.1
t=0 0
a=tool:libavformat 58.20.100
m=video 0 RTP/AVP 96
b=AS:1904
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAH6zRAFAFuwFqAgICgAAAAwCAAAAZB4wYiQ==,aOuPLA==; profile-level-id=64001F
a=control:streamid=0

```

h265的流
```
v=0
o=- 0 0 IN IP4 127.0.0.1
s=No Name
c=IN IP4 127.0.0.1
t=0 0
a=tool:libavformat 58.20.100
m=video 0 RTP/AVP 96
b=AS:805
a=rtpmap:96 H265/90000
a=fmtp:96 sprop-vps=QAEMAf//AWAAAAMAkAAAAwAAAwBdlZgJ; sprop-sps=QgEBAWAAAAMAkAAAAwAAAwBdoAKAgC0WWVmkkyvAQEAAAAMAQAAABkI=; sprop-pps=RAHBcrRiQA==
a=control:streamid=0

```
- 客户端首次发送请求，要求sdp，服务器由于客户端没有认证拒绝。
- 再次请求的时候，携带着password的摘要（应该是和nonce一起计算的结果）发生给服务器。
- 服务器通过认证后，会把sdp以body的形式返回。
- a=control是一些控制便签，其中a=control:trackID=0说明使用trackId=0来引用设备
-  sprop-parameter-sets表示pps,sps参数？？？

## SETUP
```
# 请求
SETUP rtsp://118.140.224.230:554/main/trackID=0 RTSP/1.0\r\n
Transport: RTP/AVP/TCP;unicast;interleaved=0-1
x-Dynamic-Rate: 0\r\n
CSeq: 4\r\n
Authorization: ....
\r\n

#响应
RTSP/1.0 200 OK\r\n
Transport: RTP/AVP/TCP;unicast;destination=100.104.129.160;source=118.140.224.230;interleaved=0-1;ssrc=ffef24e
Session: 444444444444444;timeout=60
\r\n
```
- setup的主要功能是确定双方的传输线路，ssrc
- 随着session id的返回，标志着会话建立完成。
- 注意到trackID=0被添加到了 url,表示客户端发起了对0轨道的控制
- 对于使用tcp传输的流，客户端通常使用interleave表示客户端希望采用交互模式，交互模式是说 之后的rtp,rtsp命令打包到一个 tcp包中，这样就实现了复用，0-1表示通道？？。
### interleaved=0-1的含义
- 0表示rtp，1表示rtcp
- 实际抓包的时候，观测到interleave发的包是rtsp+rtp+rtsp+rtp,每个rtsp都会有一个channel，表示传输的是rtp还是rtcp
```
一个数据包的组成

RTSP InterFrame(channel=0)
RTP
RTSP InterFrame(channel=0)
RTP
RTSP InterFrame(channel=0)
RTP
```
### 补充udp的setup
```
# 请求
SETUP rtsp://test1:5544/live/test110/streamid=0 RTSP/1.0
Transport: RTP/AVP/UDP;unicast;client_port=21316-21317
CSeq: 3
User-Agent: Lavf60.4.100


#响应
RTSP/1.0 200 OK
CSeq: 3
Date: Tue, 31 Oct 2023 02:46:22 UTC
Session: 191201771
Transport: RTP/AVP/UDP;unicast;client_port=21316-21317;server_port=30042-30043

```
- RTP/AVP/UDP 表示使用Udp传输
- client_port,server_port分别表示客户端，服务器使用的rtp和rtcp的端口
## Play 
```
# 请求
PLAY rtsp://118.140.224.230:554/chID=000000010-0000-0000-0000-000000000000&streamType=main&linkType=tcpst/ RTSP/1.0\r\n
CSeq: 5\r\n
Session: 444444444444444
Authorization: ....
\r\n


#响应
RTSP/1.0 200 OK\r\n
CSeq: 5\r\n
Session: 444444444444444;
RTP-Info: url=rtsp://118.140.224.230:554/main/trackID=0;seq=62030;rtptime=268366414\r\n
\r\n
```

- play 可以认为是正式开始拉流了，客户端请求，服务器给出rtp起始的seq=62030,首个rtp包生成的时间戳是268366414