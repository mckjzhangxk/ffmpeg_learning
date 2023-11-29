## mp4 box结构

```

| size | type | data |
```
- 4字节size+4字节type+data
- type也叫FourCC
- data是box的实际数据，可以是纯媒体数据，也可能是嵌套的box

## box


```
moov
   - mvhd
   - track
      - tkhd
      - mdia
        - mdhd
        - hdlr
        - minf
          - vmhd
          - smhd
          - dinf
          - stbl
        - udat
   - track 
      - tkhd
      - mdia
        - mdhd
        - hdlr
        - minf
          - vmhd
          - smhd
          - dinf
          - stbl
        - udat

```
### moov   
- moov本身不包含重要的数据，他是mvhd,track的父容器。

### mvhd
- mvhd是每天头信息，有以下重要的参数：
- - timescale(4字节):时间刻度
- - duration(4字节):以timescale为刻度
- - next trackID: 表示下一个trackId
- - 创建时间，修正时间

### track
- track和moov一样不保存数据，只作为父容器存在
- track必须包含 tkhd,mdia box.
- track分为 media track(包含媒体信息)和 hint track(包含流媒体打包信息)

### tkhd
- 每个track只有一个tkhd,包含以下重要参数
- - trackID
- - duration
- - 音量
- - width/height


### mdia
- 父容器，必须包含 mdhd,hdlr,minf

### mdhd(media header box)
- 重要参数如下
- - timescale
- - duration

### hdlr(handler renference box)
- 重要参数如下
- - handle的子类型： vide/soun
### minf(media information box)
- 父容器，包含以下重要的子容器
- - vmhd: video media information header:视频信息头
- - smhd: sound media information header:音频信息头
- - dinf: data information：
- - stbl:sample table:采样表
### udta(user data box)