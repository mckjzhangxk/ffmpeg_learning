cmake_minimum_required(VERSION 3.26)
project(webrtc)

set(CMAKE_CXX_STANDARD 11)

add_executable(webrtc main.cpp
        Server.cpp
        Server.h)
