## 本章目录
- 如何获取WebRTC源码
- - [google官网](http://webrtc.org/)
- - [声网镜像](https://webrtc.org.cn/mirror/)
- - [windows安装](https://avdancedu.com/2bafd6cf/)

- 安装必备的工具
- 如何编译WebRTC源码
- 运行一Demo试试
- 详解WebRTC的目录结构
## 下载源码
```sh
#先安装 depot_tool
git clone https://webrtc.bj2.agoralab.co/webrtc-mirror/depot_tools.git

#下载源码，这里是从官方下载
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc
gclient sync
```
## 编译
```sh
#gn相当于cmake
gn gen out/Default #产生编译脚本
gn clean out/Default #清理
gn args --list out/Default #查看全部的gn参数
gn args out/Default --list= is_debug #

#ninja相当于make
ninja -C out/Default


#最后生成vs studio的工程文件
gn gen --ide=vs outlDefault
```