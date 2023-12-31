#!/bin/bash
#在ffmpeg目录下执行该脚本，输出在ffmpeg目录下的android-build 目录里
PREFIX=/Users/zhanggxk/project/bin/android-build

#设置你自己的NDK位置
NDK_HOME=/Users/zhanggxk/project/bin/toolchain

COMMON_OPTIONS="\
    --target-os=android \
    --disable-static \
    --enable-shared \
    --enable-small \
    --disable-programs \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-doc \
    --disable-symver \
    --disable-asm \
    "
function build_android {

    ./configure \
    --prefix=${PREFIX} \
    --libdir=${PREFIX}/libs/armeabi-v7a \
    --incdir=${PREFIX}/includes/armeabi-v7a \
    --pkgconfigdir=${PREFIX}/pkgconfig/armeabi-v7a \
    --arch=arm \
    --cpu=armv7-a \
    --cross-prefix="${NDK_HOME}/bin/llvm-" \
    --cc="${NDK_HOME}/bin/armv7a-linux-androideabi29-clang" \
    --sysroot="${NDK_HOME}/sysroot/" \
    --extra-ldexeflags=-pie \
    --extra-cflags="-DVK_ENABLE_BETA_EXTENSIONS=0" \
    ${COMMON_OPTIONS}
    make clean
    make -j8 && make install


    ./configure \
    --prefix=${PREFIX} \
    --libdir=${PREFIX}/libs/arm64-v8a \
    --incdir=${PREFIX}/includes/arm64-v8a \
    --pkgconfigdir=${PREFIX}/pkgconfig/arm64-v8a \
    --arch=aarch64 \
    --cpu=armv8-a \
    --cross-prefix="${NDK_HOME}/bin/llvm-" \
    --cc="${NDK_HOME}/bin/aarch64-linux-android29-clang" \
    --sysroot="${NDK_HOME}/sysroot/" \
    --extra-ldexeflags=-pie \
    --extra-cflags="-DVK_ENABLE_BETA_EXTENSIONS=0" \
    ${COMMON_OPTIONS} 
    make clean
    make -j8 && make install

    ./configure \
    --prefix=${PREFIX} \
    --libdir=${PREFIX}/libs/x86 \
    --incdir=${PREFIX}/includes/x86 \
    --pkgconfigdir=${PREFIX}/pkgconfig/x86 \
    --arch=x86 \
    --cpu=i686 \
    --cross-prefix="${NDK_HOME}/bin/llvm-" \
    --cc="${NDK_HOME}/bin/i686-linux-android29-clang" \
    --sysroot="${NDK_HOME}/sysroot/" \
    --extra-ldexeflags=-pie \
    --extra-cflags="-DVK_ENABLE_BETA_EXTENSIONS=0" \
    ${COMMON_OPTIONS} 
    make clean
    make -j8 && make install

    ./configure \
    --prefix=${PREFIX} \
    --libdir=${PREFIX}/libs/x86_64 \
    --incdir=${PREFIX}/includes/x86_64 \
    --pkgconfigdir=${PREFIX}/pkgconfig/x86_64 \
    --arch=x86_64 \
    --cpu=x86_64 \
    --cross-prefix="${NDK_HOME}/bin/llvm-" \
    --cc="${NDK_HOME}/bin/x86_64-linux-android29-clang" \
    --sysroot="${NDK_HOME}/sysroot/" \
    --extra-ldexeflags=-pie \
    --extra-cflags="-DVK_ENABLE_BETA_EXTENSIONS=0" \
    ${COMMON_OPTIONS}
    make clean
    make -j8 && make install
};

build_android
