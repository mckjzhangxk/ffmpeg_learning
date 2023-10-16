## 基本概念
- worker
- router
- producer/consumer
- transport:多个consumer可以共享transport
- - WebrtcTransport,PlainTtpTransport,PipeTransport
![ms.png](images%2Fms.png)
- ## 包括的特征
- 支持IPV6
- ICE,RTP,RTCP,DTLS 在 UDP与TCP协议基础之上
- 支持Simulcast(多路流)和SVC(多层流)
- 支持拥塞控制
- 优秀的带宽评估
- 支持STCP协议
- 多路流可复用 ICE+DTLS传输通道
## C++的类图
- 继承，包含,使用
- Remb是带宽评估算法
-  TransportTuple 是个三元组,<本地，远端，协议>
![c++_mediasoup.png](images%2Fc%2B%2B_mediasoup.png)
![ms_c++2.png](images%2Fms_c%2B%2B2.png)
## Js的类图
- 管理层


## Media是如何启动的
- 在js层， 调用 mediasoup.createWorker
- Js的Worker的构造函数中，调用了nodeJS提供的spawn()方法，加载了一个二进制mediasoup（node_modules/mediasoup/worker/out/Release/mediasoup-worker）
- 在调用mediasoup-worker的时候，重定向了如下的（标准IO）,这些管道都是全双工的（ChannelSocket）的。
```js
            // fd 0 (stdin)   : Just ignore it.
            // fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
            // fd 2 (stderr)  : Same as stdout.
            // fd 3 (channel) : Producer Channel fd.
            // fd 4 (channel) : Consumer Channel fd.
            // fd 5 (channel) : Producer PayloadChannel fd.
            // fd 6 (channel) : Consumer PayloadChannel fd.
            stdio: ['ignore', 'pipe', 'pipe', 'pipe', 'pipe', 'pipe', 'pipe']
```
- 在C++层，会尝试打开 3，4，5，6这4个channel，如果打不开就会报错。