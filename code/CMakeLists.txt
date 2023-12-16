cmake_minimum_required(VERSION 3.15)
project(code)


find_package(PkgConfig REQUIRED)

#set(CMAKE_CXX_STANDARD 14)

include_directories(/Users/zhanggxk/project/bin/SDL/include/SDL2)
link_directories(/Users/zhanggxk/project/bin/SDL/lib)


pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libavdevice libswscale libswresample libavfilter)
message("---Find FFmpeg library")
message("---include path: ${FFMPEG_INCLUDE_DIRS}")
message("---library path: ${FFMPEG_LIBRARY_DIRS}")
message("------lib names:     ${FFMPEG_LIBRARIES}")


pkg_check_modules(RTMP REQUIRED librtmp)
message("---Find RTMP library")
message("---include path: ${RTMP_INCLUDE_DIRS}")
message("---library path: ${RTMP_LIBRARY_DIRS}")
message("------lib names: ${RTMP_LIBRARIES}")


pkg_check_modules(X264 REQUIRED x264)
message("---Find X264 library")
message("---include path: ${X264_INCLUDE_DIRS}")
message("---library path: ${X264_LIBRARY_DIRS}")
message("------lib names: ${X264_LIBRARIES}")


#头文件与库文件检索路径
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})
include_directories(${RTMP_INCLUDE_DIRS})
link_directories(${RTMP_LIBRARY_DIRS})

include_directories(${X264_INCLUDE_DIRS})
link_directories(${X264_LIBRARY_DIRS})

#常用工具
add_executable(ffmpeg_log basic/ffmpeg_log.cpp)
add_executable(ffmpeg_dir basic/ffmpeg_dir.cpp)
add_executable(mediainfo basic/mediainfo.cpp)
add_executable(time_base basic/timebase.cpp)


#解复用相关操作
add_executable(remuxing basic/remux/remuxing.cpp)
add_executable(remuxing-h264 basic/remux/remuxing-h264.cpp)
add_executable(remuxing-from-memory basic/remux/remuxing-from-memory.cpp)
add_executable(extractAudio basic/remux/extractAudio.cpp)
add_executable(extractVideo basic/remux/utils.c basic/remux/extractVideo.c)
add_executable(cutVideo basic/remux/cutVideo.cpp)
add_executable(mp4parser basic/remux/mp4parser.cpp)

#设备的操作
add_executable(device_open device/opendevice.cpp)
add_executable(device_record device/record_audio.cpp)
add_executable(device_record_video device/record_video.cpp)

#重采样相关
add_executable(resample_basic basic/resample/resample_basic.cpp)
add_executable(resample_video basic/resample/resample_video.cpp)
add_executable(audio_resample basic/audio_resample.cpp)
#filter相关
add_executable(drawbox_filter basic/filter/drawbox_filter.cpp)

add_executable(concat excercise/concat.c)
add_executable(split excercise/split.c)
add_executable(merge excercise/merge.c)
add_executable(codecEncoderH264 basic/encode_decode/encode_video.cpp)
add_executable(codecEncoderAAC excercise/audioEncoder.cpp)
add_executable(decodeVideo basic/decodeVideo.cpp)
add_executable(codecEncoderAndMutex excercise/codecEncoderAndMutex.cpp)


#SDL开发
add_executable(first_sdl sdl/first_sdl.c)
add_executable(sdltexture sdl/texture.c)
add_executable(yuv_player sdl/yuv_player.c)
add_executable(pcm_player sdl/pcm_player.c)
add_executable(simple_player sdl/simple_player.c)
add_executable(simple_player2 sdl/simple_player2.cpp)
add_executable(thread_player sdl/thread_player.cpp)
add_executable(thread_player_video_only sdl/thread_player_video_only.cpp)
#add_executable(sdlrect sdl/rect.c)

#流媒体相关
add_executable(rtmp_stream live_stream/rtmp_stream.cpp)

#基础相关
target_link_libraries(ffmpeg_log  ${FFMPEG_LIBRARIES})
target_link_libraries(ffmpeg_dir  ${FFMPEG_LIBRARIES})
target_link_libraries(mediainfo   ${FFMPEG_LIBRARIES})


#复用 解复用相关
target_link_libraries(remuxing  ${FFMPEG_LIBRARIES})
target_link_libraries(remuxing-h264  ${FFMPEG_LIBRARIES})
target_link_libraries(remuxing-from-memory  ${FFMPEG_LIBRARIES})
target_link_libraries(extractAudio  ${FFMPEG_LIBRARIES})
target_link_libraries(extractVideo  ${FFMPEG_LIBRARIES})
target_link_libraries(cutVideo  ${FFMPEG_LIBRARIES})

#设备的操作
target_link_libraries(device_open   ${FFMPEG_LIBRARIES})
target_link_libraries(device_record ${FFMPEG_LIBRARIES} )
target_link_libraries(device_record_video ${FFMPEG_LIBRARIES} )

#重采样相关
target_link_libraries(resample_basic ${FFMPEG_LIBRARIES})
target_link_libraries(resample_video ${FFMPEG_LIBRARIES})
target_link_libraries(audio_resample ${FFMPEG_LIBRARIES})

#filter相关
target_link_libraries(drawbox_filter ${FFMPEG_LIBRARIES})


target_link_libraries(concat  ${FFMPEG_LIBRARIES})
target_link_libraries(split  ${FFMPEG_LIBRARIES})
target_link_libraries(merge  ${FFMPEG_LIBRARIES})
target_link_libraries(codecEncoderH264 ${FFMPEG_LIBRARIES})
target_link_libraries(codecEncoderAAC ${FFMPEG_LIBRARIES})
target_link_libraries(decodeVideo ${FFMPEG_LIBRARIES})


target_link_libraries(codecEncoderAndMutex ${FFMPEG_LIBRARIES})
target_link_libraries(time_base ${FFMPEG_LIBRARIES})


#SDL开发
target_link_libraries(first_sdl SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(sdltexture SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(yuv_player SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(pcm_player SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(simple_player SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(simple_player2 SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(thread_player SDL2 ${FFMPEG_LIBRARIES})
target_link_libraries(thread_player_video_only SDL2 ${FFMPEG_LIBRARIES})

#流媒体相关
target_link_libraries(rtmp_stream ${RTMP_LIBRARIES})




add_executable(packet_view excercise/packet_view.cpp)
target_link_libraries(packet_view ${FFMPEG_LIBRARIES})


add_executable(h264ToTs excercise/h264ToTs.cpp)
target_link_libraries(h264ToTs ${FFMPEG_LIBRARIES})



add_executable(encode_with_libx264 basic/encode_decode/encode_with_libx264.cpp)
target_link_libraries(encode_with_libx264 ${X264_LIBRARIES})








