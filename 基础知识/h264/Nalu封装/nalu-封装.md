RTP对H264(NALU)的封装格式

为什么要封装？为什么使用FU-A？

由于UDP传输有MTU的限制，使用RTP传输H264的时候，往往一个NALU包大小超过MTU，
这就需要借助封包技术，FU-A是常见的封装格式。

## FU-A(type 28)
- 多个RTP包组成一个NALU
- FU-A封装加入了Fu_indicator,FU_header字段
- Fu_indicator字段主要标识 NALU封装的类型
- FU_header 包括 NALU的类型，以及这个nalu是否开始，是否结束

**注意：Fu  Header VS  Nalu Header**

- FU_header其实就是NALU的header，严格的说不是FU封包加入的字段，而是复用了NALU的header，改变了位的含义


| 位   | Nalu   | FU-A          |
|-----|--------|---------------|
| 0   | forbid | start，Nalu的起始 |
| 1   | 重要性    | end，Nalu的结束   |
| 2   | 重要性    | 保留位           |
| 3-7 | 帧的类型   | 帧的类型          |
请注意，对H264的封装有很多种，比如ts,annexb,avc，而FU-A是配合使用RTP传输
H264时候的封装格式。

- 既然NALU的header的都三位被赋予了其他含义，Fu追加了一个封装indicator，用于弥补这一信息

Fu_indicator结构

| 位   | FU-A   | 含义            |
|-----|--------|---------------|
| 0   | forbid | start，Nalu的起始 |
| 1   | 重要性    | end，Nalu的结束   |
| 2   | 重要性    | 保留位           |
| 3-7 | 28     | FU-A封装类型     |