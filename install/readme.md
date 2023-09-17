# ubuntu下编译
```s
apt install libfdk-aac-dev
```

```s
./configure --enable-libmp3lame --enable-libx264 --enable-gpl --enable-libopus --enable-libvpx --enable-libfdk-aac --enable-nonfree --enable-shared --disable-stripping --enable-zlib --enable-avresample --enable-decoder=png

make && make install
```



安装位置：
- /usr/local/lib
- /usr/local/include



libavcodec.a            libavformat.a      libswresample.a    
 libavfilter.a              libavutil.a               libpostproc.a            
libavdevice.a          libavresample.a     libswscale.a              




# centos7 32位
```sh
./configure --arch=x86 --prefix=/Users/zhanggxk/project/bin --cc=/usr/bin/clang  --enable-gpl --enable-shared --extra-cflags=-m32 --extra-ldflags=-m32 --enable-cross-compile

```

```
./configure \
--prefix=/home/hack520/ffmpeglib \
--cc=/usr/bin/clang \
--disable-vaapi \
--enable-gpl \
--enable-nonfree \
--enable-shared \
--enable-static \
--enable-libfdk-aac \
--enable-libx264 \
--enable-libx265 \
--enable-ffplay

```

# mac下编译
```sh
./configure --prefix=/Users/zhanggxk/project/bin/ffmpeg4.1_aac \
--cc=/usr/bin/clang  \
--enable-gpl \
--enable-nonfree \
--enable-shared \
--enable-libfdk-aac \
--enable-libopus \
--enable-libx264 \
--enable-libx265 \
--enable-debug=3 \
--enable-ffplay
```
安装opus,fdk-aac,x264库
```sh
brew install fdk-aac
brew  install opus
brew install x264 x265
brew install pkg-config

# 
brew --prefix opus
brew --prefix fdk-aac

设置PKG_CONFIG_PATH

PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/homebrew/opt/fdk-aac/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/homebrew/opt/opus/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/homebrew/opt/x264/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/homebrew/opt/x265/lib/pkgconfig


```

```
./configure \
--prefix=/Users/zhanggxk/project/bin/ffmpeg4.1_aac \
--cc=/usr/bin/clang  \
--enable-gpl \
--enable-nonfree \
--enable-shared \
--enable-libfdk-aac \
--enable-libopus \
--enable-libx264 \
--enable-libx265 \
--extra-ldflags="-L /opt/homebrew/opt/fdk-aac/lib -L /opt/homebrew/opt/opus/lib -L /opt/homebrew/opt/opus/lib -L /opt/homebrew/opt/x264/lib -L /opt/homebrew/opt/x265/lib" \
--extra-cflags="-I /opt/homebrew/opt/fdk-aac/include -I /opt/homebrew/opt/opus/include -I /opt/homebrew/opt/x264/include -I /opt/homebrew/opt/x265/include"


```



