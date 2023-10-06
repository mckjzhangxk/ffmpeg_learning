## 什么是sdp?
- 是一种信息格式描述标准，本身不属于传输协议.
- 一个会话级的描述
- 多个媒体级的描述
- 由多个 <type>=<valye>组成

## 会话层
- 会话的名称和目的
- 会话的存活时间
- 会话中包括多个媒体信息
```sh
v=0  #协议版本
#o=<username> <session id> <version> <network type> <address type> <address>
o=msms 86019764 1 IN IP4 0.0.0.0 #所有者/会话描述符

#c=<network type> <address type> <address>
c=IN IP4 127.0.0.1 #会话层的连接信息，如果[媒体层]不设置，就使用这里的

s=- #会话的名称,-表示忽略
t=0 0 #会话存活时间，0表示永久

```
## 媒体层
- 媒体格式
- 传输协议
- 传输ip和端口
- 媒体的负载类型

```sh
#m=<media> <port> <transport> <fmt/payload type list>
m=video 9 UDP/TLS/RTP/SAVPF 106 107 108 109 116 117 118
m=audio 9 UDP/TLS/RTP/SAVPF 111 63 13
m=application 9 UDP/DTLS/SCTP webrtc-datachannel

c=IN IP4 0.0.0.0 #媒体层的 连接信息，覆盖【会话层的】
b=AS:500  #带宽，用于限流使用
```
- 对于trasport UDP/TLS/RTP/SAVPF如下解读，dtls,srtp,SAVPF表示传输的内容(secure audio video profile feedback)

### Attribute
```sh
a=<Type>
a=<Type>:<values>

a=mid:0
a=sendonly
```

### rtpmap
- 对rtm的配置
```sh
a=rtpmap:<fmt/payload type> <encoding name>/<clock rate>/<encoding parameters>

a=rtpmap:106 H264/90000
a=rtpmap:107 rtx/90000 #表示重传协议
```

### fmtp
- format parameters,配置payload的其他参数
```sh
a=fmtp:<format/payload type> parameters

a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f #表示 h264的level
a=fmtp:107 apt=106 #表示106是107的关联协议
```

## webrt 中的sdp
- 会话层:v=,o=,t=
- 网络描述：c=,a=candidate
```sh
a=candidate:3330299874 1 udp 1076558079 43.128.61.219 16285 typ host generation 0
a=candidate:3330299874 1 tcp 1076302079 43.128.61.219 16285 typ host generation 0
a=end-of-candidates


a=setup:active #作为客户端
a=setup:passive #作为服务端
a=setup:actpass #2者都可以
```
- 流描述：m=,a=rtpmap,a=fmtp,a=msid-semantic:
```sh
#传输的stream id
a=msid-semantic: WMS 2be128ba-1d03-42bf-ab0e-7629d9eac16e


#流id +track id
m=...
a=mid:0
a=msid:2be128ba-1d03-42bf-ab0e-7629d9eac16e 143c8ad2-45fc-45de-aa91-a4e80ec05052

# ssrc关联的cname,cname是唯一的一个名字
a=ssrc:115004929 cname:duaddjrw93wG+Dz4
# ssrc他关联到那个track
a=ssrc:115004929 msid:2be128ba-1d03-42bf-ab0e-7629d9eac16e 143c8ad2-45fc-45de-aa91-a4e80ec05052

```
- 安全描述: ice-ufrag,ice-pwd是 一端向另一端通过stun发起连接时候携带的用户名密码，fingerprint是 证书的签名摘要，防止证书被篡改
```sh
# ice建立connectin发送的认证信息
a=ice-ufrag:89c0cnpgvxqgqknw
a=ice-pwd:tetplge5s6jxflktazeeed3y97smgep2

#dtls握手交换的证书的指纹，对端用这个来验证
a=fingerprint:sha-256 2E:B8:64:CF:65:65:A5:80:A8:37:DC:B0:99:EB:41:89:3D:18:4C:7D:FD:B2:D3:B2:D0:06:D0:73:6A:BD:2B:EE

```

- 服务质量
```sh
#表示0，1，2号媒体流复用端口,与a=mid对应
a=group:BUNDLE 0 1 2

a=rtcp-fb:106 goog-remb
#表示rtcp与rtp复用端口
a=rtcp-mux

#数据的加密算法
a=crypto ....
```

- extmap：如果rtp header设置了扩展位，这里是对扩展位的描述。
```
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id

```