## 环境相关问题
- <font color=red>源码安装</font>

```shell
# 1.下载源码，contrib表示opencv的贡献功能
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib


# 2.配置
#目录结果 opencv,opencv_contrib,build
#https://velog.io/@jinhasong/OpenCV-4.4
cmake -S opencv \
      -B build \
      -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=/Users/zhanggxk/project/bin/opencv \
      -D OPENCV_EXTRA_MODULES_PATH=opencv_contrib/modules \
      -D WITH_FFMPEG=ON \
      -D WITH_IPP=ON
      
3.安装
make -j
make install
```

-  解决pycharm提示问题
```sh
#https://blog.csdn.net/m0_57110410/article/details/125531873

1.文件---->设置---->项目---->Python解释器
2.点击右侧齿轮---->全部显示
3.选择你的解释器---->点击显示所选解释器的路径
4.把cv2文件夹添加进解释器路径里
  cv2文件夹：anaconda3/lib/python3.10/site-packages/cv2
```
- 命令行安装
```sh
pip install opencv-python
```

## 基础操作
- [ 创建和显示窗口与加载图片](basic/win.py)
- [视频采集/播放](basic/capture.py)
- [视频录制](basic/video_record.py)
- [鼠标控制](basic/control.py)
- [TrackBat](basic/trackbar.py)