cmake_minimum_required(VERSION 3.26)
project(webrtc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
include_directories(${LIBEVENT_INCLUDE_DIRS})
link_directories(${LIBEVENT_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 11)

add_executable(webrtc main.cpp
        basic/Server.cpp
        basic/Server.h)


#信号安全
add_executable(signal basic/signal/signal.cpp)
add_executable(signal_unsafe basic/signal/signal_unsafe.cpp)

#后台运行
add_executable(daemon_process basic/daemon_process.cpp)

#tcp,udp服务器
add_executable(tcp_server basic/tcp/tcp_server.cpp)
add_executable(tcp_client basic/tcp/tcp_client.cpp)
add_executable(udp_server basic/tcp/udp_server.cpp)
add_executable(udp_client basic/tcp/udp_client.cpp)
#异步IO复用
add_executable(tcp_servver_select basic/async/tcp_server_select.cpp)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux操作系统
    add_executable(tcp_server_epoll basic/async/tcp_server_epoll.cpp)
    # 在这里添加Linux系统下的指令
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS操作系统
    message("This is macOS.")
endif ()
add_executable(tcp_server_libevent basic/async/tcp_server_libevent.cpp)

target_link_libraries(tcp_server_libevent ${LIBEVENT_LIBRARIES})

add_executable(sem_test basic/sem/sem_test.cpp)
target_link_libraries(sem_test pthread)