## RTP
```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       Sequence Number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             SSRC                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     CSRC (if any)                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       RTP Payload Data                        |
|                            ...                                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

- P字段：代表是否padding,如果P=1，包的末尾字节表示填充的字节数（包括末尾字节）。
- X:是否扩展
- CC： CSRC计数器
- M(Marker)：含义取决于应用程序。视频表示是否是视频帧的结束,音频标准起始
- PT:payload type,payload type会关联一个clock Rate.96以上是自定义的PT
- <font color=red>Sequence Number :连续的需要，每次加1.用于对数据包拍戏</font>
-  <font color=pink>Timestamp ：生成RTP包的时候一个相对的时间戳，时间基由PT.clock Rate决定,分包后的数据包，每个包中timestamp都一样</font>
- SSRC(源的标识符)：一般标识 一路音频或者一路视频. 
- CSRC(共享源的标识符)：用于标识参与生成RTP数据包的各个贡献源。CSRC计数器字段指示了CSRC列表的长度。

- RTP头部是12字节，不包括CSRC.
## RTCP
```
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|   RC    |  PT   |        length                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

- V（Version）：2位，指示RTCP协议的版本。
- P（Padding）：1位，指示RTCP报文是否包含填充字节,报文末尾是填充字节数。
- RC（Report Count）：5位，指示RTCP报文中报告块的数量。
- PT（Packet Type）：8位，指示RTCP报文的类型，如SR、RR、SDES、BYE等。
- length（长度）：16位，指示RTCP报文的总长度，包括4字节的头部。


### Packet Type

|PT |标识| 全名| 备注|
|- | -  | -  |-   |
|200 | SR | Sender Report   | 带宽评估有关   |
|201 | RR | Receiver Report  |带宽评估有关   |
|202 | SDES | Source Description  |最重要的是cname   |
|203 | BYE | Goodbye  |   |
|204 | APP |  Application-defined packet |   |用户自定义反馈
|192 | FIR | Full INTRA-frame Request  | 废弃，请求IDR  |
|192 | NACK | Negative Acknowledgement. | 废弃，格式是Seq+mask，mask置1的地方表示没有收到的seq number  |
|205 | RTPFB | Generic RTP Feedback  |  用于传输RTP特定的反馈信息，如Generic NACK等。 |
|206 | PSFB | Payload-specific Feedback  | 用于传输负载特定的反馈信息，如NACK（Negative Acknowledgment）等。 |