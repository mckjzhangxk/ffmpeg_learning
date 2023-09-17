//
// Created by zzzlllll on 2021/11/2.
//

#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

//clang
//  -I /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/include
//  -L /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/lib
//  -lavutil -lavformat
//  -o extractAudio extractAudio.c

//av_find_best_stream
//av_read_frame,av_packet_unref



//抽取视频流需要掌握以下知识点

//1 start code :特征码，用于标识frame与frame的分割
//2 pps/sps,简单的理解就是标识 w,h,fps...
//3. codec->data中可以获得 pps,sps


//问题，pps/sps全局就一个可以吗？

//不可以， 对于文件(AVC)，每次分辨率改变前，都需要重新记录新的sps/pps
//不可以, 对于网络流(AnnexB),的pps/sps全一旦丢失，就会到时之后的所有解码异常
//所以网络流的结果是如下的：
// [start code][sps][pps][frame]  [start code][sps][pps][frame]  [start code][pps][sps][frame]

//值得注意的是，抽取出来的h264由于没有头部设置FPS，播放器会按照25fps播放
int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *in, FILE *dst_fd);
int main(int argc, char *argv[]) {

    av_log_set_level(AV_LOG_INFO);
//    av_register_all();

    if (argc < 3) {
        av_log(NULL, AV_LOG_WARNING, "usage: extractVideo src dst\n");
        return -1;
    }
    char *src = argv[1];
    char *dst = argv[2];
    //1.打开源文件，目标文件

    AVFormatContext *ctx = NULL;

    int resultCode = avformat_open_input(&ctx, src, NULL, NULL);
    if (resultCode < 0) {
        goto end;
    }
    //打印基本文件信息，第四个参数是 0-输入 1-输出
    av_dump_format(ctx, 0, src, 0);

    //output acc file
    FILE * dst_fd=fopen(dst,"wb");
    if(!dst_fd){
        av_log(NULL, AV_LOG_ERROR, "can't open dst file %s", dst);
        goto end;
    }

    //2.获得要抽取的流
    //对于ts文件，需要调用avformat_find_stream_info,才可以读取到流，这是为什么？
    if((resultCode = avformat_find_stream_info(ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to find stream information: %s, %d(%s)\n",
               src,resultCode,av_err2str(resultCode));
        return -1;
    }
    //第三个参数是需要的 流 索引(ffprobe)，第四个是相关索引
    //related_stream和 ts的节目表有关。。。。
    int videoIndex=av_find_best_stream(ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(videoIndex<0){
        resultCode=videoIndex;
        goto end;
    }

    //3.读取package,输出到文件
    AVPacket packet;
    av_init_packet(&packet);

    while(av_read_frame(ctx,&packet)>=0){
        if(packet.stream_index==videoIndex){
//            fwrite(packet.data,packet.size,1,dst_fd);
            h264_mp4toannexb(ctx,&packet,dst_fd);
        }
        //减少引用计数
        av_packet_unref(&packet);
    }

    end:
    if(dst_fd)
        fclose(dst_fd);
    if (ctx)
        avformat_close_input(&ctx);

    if (resultCode) {
        av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(resultCode));
    }
    return resultCode;
}

