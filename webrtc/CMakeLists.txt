cmake_minimum_required(VERSION 3.26)
project(webrtc)

set(CMAKE_CXX_STANDARD 11)

add_executable(webrtc main.cpp
        basic/Server.cpp
        basic/Server.h)



add_executable(signal basic/signal/signal.cpp)
add_executable(signal_unsafe basic/signal/signal_unsafe.cpp)
add_executable(daemon_process basic/daemon_process.cpp)

add_executable(tcp_server basic/tcp/tcp_server.cpp)
add_executable(tcp_client basic/tcp/tcp_client.cpp)



add_executable(tcp_servver_select basic/async/tcp_server_select.cpp)