cmake_minimum_required(VERSION 3.26)
project(webrtc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
include_directories(${LIBEVENT_INCLUDE_DIRS})
link_directories(${LIBEVENT_LIBRARY_DIRS})

include_directories(/Users/zhanggxk/project/bin/cppcms/include)
link_directories(/Users/zhanggxk/project/bin/cppcms/lib)


include_directories(/Users/zhanggxk/project/bin/libcurl/include)
link_directories(/Users/zhanggxk/project/bin/libcurl/lib)


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
add_executable(raw_udp_socket basic/socket/raw_udp_socket.cpp)


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

#stun服务
add_executable(stun_server stun/stun_server.cpp
        stun/utils.cpp)
add_executable(stun_client stun/stun_client.cpp
        stun/utils.cpp)



#cppcms
add_executable(hellocms cppcms_tutorial/hello.cpp)
target_link_libraries(hellocms cppcms)

add_executable(mappingcms cppcms_tutorial/mapping.cpp)
target_link_libraries(mappingcms cppcms)

add_executable(hierarchy cppcms_tutorial/hierarchy.cpp)
target_link_libraries(hierarchy cppcms)


add_executable(json_cms cppcms_tutorial/json_cms.cpp)
target_link_libraries(json_cms cppcms)


add_executable(server cppcms_tutorial/server.cpp)
target_link_libraries(server cppcms)


add_executable(curl_tutorial libcurl_tutorial/curl_tutorial.cpp)
target_link_libraries(curl_tutorial curl)

#rtsp 插播
add_executable(socket_pair basic/pipe/socketpair_demo.cpp)
add_executable(rtsp_server rtsp_server/rtsp_server.cpp)
add_executable(rtsp_tcp_server rtsp_server/rtsp_tcp_server.cpp)
add_executable(rtsp_tcp_server_h265 rtsp_server/rtsp_tcp_server_h265.cpp)
add_executable(proxy rtsp_server/network_io.cpp rtsp_server/proxy.cpp)
add_executable(proxy_tvt rtsp_server/network_io.cpp rtsp_server/proxy_tvt.cpp)


add_executable(getopt_tutorial basic/filesystem/getopt_tutorial.cpp)