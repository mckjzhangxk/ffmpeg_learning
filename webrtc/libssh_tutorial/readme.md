## mbedtls
- linux编译的时候可能出现 prototype的错误(-Wstrict-prototypes)，需要把没有参数的函数加上void
- 需要编译thread的支持
- linux编译的时候可能出现不支持直接对象 成员内部的错误，需要开启对上面的支持
- [mbedtls配置](https://mbed-tls.readthedocs.io/en/latest/kb/compiling-and-building/how-do-i-configure-mbedtls/)
- [mbedtls线程模型](https://mbed-tls.readthedocs.io/en/latest/kb/development/thread-safety-and-multi-threading/)
```shell
#1.编辑 include/mbedtls/mbedtls_config.h，加入
#define MBEDTLS_THREADING_C
#define MBEDTLS_THREADING_PTHREAD


#2.编辑 include/mbedtls/private_access.h
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#3.编译

mkdir build
cd build
mkdir <install_dir>
cmake -DCMAKE_INSTALL_PREFIX=<mtls_include_dir> ..
make install
```

## libssh

```shell
#编辑CMakeLists.txt，加入：
set(MBEDTLS_INCLUDE_DIR "<mtls_include_dir:>")
#设置环境变量
export MBEDTLS_ROOT_DIR=<mtls_install_dir>

cmake  -DWITH_MBEDTLS=ON \
      -DBUILD_SHARED_LIBS=OFF  \
      -DCMAKE_INSTALL_PREFIX=<install_dir> \
      -DWITH_ZLIB=OFF \
      -DWITH_GSSAPI=OFF  \
      -DWITH_SFTP=OFF \
      ..
 
#测试
gcc -I /root/project/libssh-0.10.0/build/install/include/ \
    -L /root/project/libssh-0.10.0/build/install/lib64/ \
    -L /root/project/mbedtls/build/install/lib64/ \
    ssh_tutorial.c  \
    -lssh  -lmbedx509 -lmbedcrypto -lmbedtls
```



