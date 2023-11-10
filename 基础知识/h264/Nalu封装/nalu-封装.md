RTP对H264(NALU)的封装格式

为什么要封装？为什么使用FU-A？

由于UDP传输有MTU的限制，使用RTP传输H264的时候，往往一个NALU包大小超过MTU，
这就需要借助封包技术，FU-A是常见的封装格式。

## FU-A(type 28)
- 如果一个NALU过大，他就需要拆包，FU-A就是一种常用的拆包的封装格式
- FU-A封装加入了Fu_indicator,FU_header字段
- <font color=red>Fu_indicator就是Nalu裸流的 第一个字节，Fu_indicator的type=28。
- Fu_indicator后面还需要一个Fu_header对象，标志nalu的起始</font>

**注意：Fu  Header VS  Nalu Header**

- FU_indicator其实就是NALU的header,Fu_indicator的type=28,换句话说 原来的一个nalu header变成了 FU_indicator+Fu_header

Fu_indicator结构

| 位   | FU-A   | 含义            |
|-----|--------|---------------|
| 0   | forbid |  |
| 1   | 重要性    | end，Nalu的结束   |
| 2   | 重要性    | 保留位           |
| 3-7 | 28     | FU-A封装类型     |

Fu_header结构
| 位   |     FU-A          |
|-----|---------------|
| 0   |   start，Nalu的起始 |
| 1   |      end，Nalu的结束   |
| 2   |      保留位           |
| 3-7 |     帧的类型          |

请注意，对H264的封装有很多种，比如annexb,avc，而FU-A是配合使用RTP传输
H264时候的封装格式。



```
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | FU indicator  |   FU header   |                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
  |                                                               |
  |                         Nalu payload                            |
  |                                                               |
  |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                               :...OPTIONAL RTP padding        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


```

[参考项目](https://gitee.com/zhoujiabo/audio-and-video-development/tree/master/rtsp_prj/rtsp_tcp_server)