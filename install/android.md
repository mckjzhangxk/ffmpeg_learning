## 准备好工具链

```sh
cd /Users/zhanggxk/Library/Android/sdk/ndk/26.0.10792818

build/tools/make_standalone_toolchain.py \
    --force \
    --arch arm \
    --api 24 \
    --install-dir=/Users/zhanggxk/project/bin/toolchain
```

```
bin:编译工具
sysroot:android的系统目录
include
lib:C++标准库等。。。
```
- 这里就是把$NDK/toolchains/llvm/prebuilt/darwin-x86_64的文件全部拷贝到install-dir下面，已经不推荐使用了


## [执行编译脚本](build_ffmpeg_for_android.sh)
```sh
    ./configure \
    --prefix=${PREFIX} \
    --libdir=${PREFIX}/libs/armeabi-v7a \        #输出的libs位置
    --incdir=${PREFIX}/includes/armeabi-v7a \    #输出的include位置
    --pkgconfigdir=${PREFIX}/pkgconfig/armeabi-v7a \  #输出的pkgconfig位置
    --arch=arm \
    --cpu=armv7-a \
    --cc="${NDK_HOME}/bin/armv7a-linux-androideabi29-clang" \  #指定编译器！
    --cross-prefix="${NDK_HOME}/bin/llvm-" \
    --sysroot="${NDK_HOME}/sysroot/" \   #android系统目录
    --extra-ldexeflags=-pie \
    --extra-cflags="-DVK_ENABLE_BETA_EXTENSIONS=0" \  #禁用beta的vulan的引入
    ${COMMON_OPTIONS}                    #ffmpeg编译选项
```

[参考构建项目](https://github.com/d-fal/ffmpeg4Android)

[参考2](https://www.icodebang.com/article/356379)