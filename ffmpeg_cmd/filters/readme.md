## 查询filter
```sh
#查看支持的filter
ffmpeg -v quiet -filters

#查看drawbox的参数
ffmpeg -h filter=drawbox
```

## filter的语法
```sh
#
ffmpeg -i in.mp4 -vf "{filter_name}={parameters}"
```
- ":"分隔参数，参数名称可以省略。
- ","分隔串联的filter
- ";"分隔 没有关联的 filter

```sh
#简单的画边框的filter
ffplay -i "/Users/zhanggxk/Desktop/测试视频/jojo.mp4" -vf "drawbox=x=10:y=10:w=100:h=100:color=red"
#可以省略掉参数
ffplay -i "/Users/zhanggxk/Desktop/测试视频/jojo.mp4" -vf "drawbox=10:10:100:100:red"
```


```sh
#添加水印的filter

#使用的三个filters
ffmpeg -h filter=movie
ffmpeg -h filter=sacle
ffmpeg -h filter=overlay

#movie -> scale, 输出别名[wm]
# [in]代表每一帧
#overlay是多输入的，[in] [wm]
ffplay -i /Users/zhanggxk/Desktop/测试视频/jojo.mp4 \
-vf "movie=filename=imgs/logo.png,scale=w=30:h=30[wm];[in] [wm]overlay=x=30:y=50"
```

```sh
# 2倍速播放
ffplay -i /Users/zhanggxk/Desktop/测试视频/jojo.mp4 \
-vf "setpts=0.5*PTS"
```