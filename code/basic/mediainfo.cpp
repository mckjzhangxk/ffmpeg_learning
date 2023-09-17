#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif
#include "libavutil/log.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libavutil/timestamp.h"
#ifdef __cplusplus
}
#endif


//clang -o mediainfo mediainfo.c -I /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/include -L /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/lib -lavutil -lavformat

//avformat_open_input,avformat_close_input
//av_dump_format

//  基本概念
// 1.多媒体文件其实就是个多媒体容器(AVFormatContext)
// 2.多媒体容器 里面是不同的stream/track
// 3.stream是不同编码器的编码结果(AVStream)
// 4.从stream读出来的是package(AVPackage)
// 5.package由 一到多frame组成

//29.97 tbr, 30k tbn表示时间基！！！
//时间基！=nu/de 是 nu/de秒！！！

// AVFormatContext 表示多媒体容器 I/O的上下文，比如说复用与解复用
// 先 打开 ，后dump，最后关闭
//https://ffmpeg.org/doxygen/4.1/group__lavf__misc.html#gae2645941f2dc779c307eb6314fd39f10

//参数
//../file/abc-0.ts ../file/abc.mp4 ../file/abc.avi "rtsp://admin:123456@114.34.141.233:554/chID=000000012-0000-0000-0000-000000000000&streamType=main&linkType=tcpst"

//AVFormatContext的重要参数
//1) duration 微秒
//2) iformat,oformat输入和输出的格式(封装)信息
//3)stream,nb_streams 流信息
//4) bit_rate


//AVStream的重要参数
// codecs
// time_base 编解码所用的时间基(tbr,tbn)
// r_frame_rate：fps

//学习文章
//https://wanggao1990.blog.csdn.net/article/details/114317618?spm=1001.2014.3001.5502

void printCodec(AVCodecContext * cxt){
    if(cxt->codec_type==AVMEDIA_TYPE_VIDEO){
        av_log(NULL,AV_LOG_INFO,"视频 %s %dx%d pixfmt(%d) 时间基：%d/%d  fps:%d\n",
               avcodec_get_name(cxt->codec_id),cxt->width,cxt->height,cxt->pix_fmt,cxt->time_base.num,cxt->time_base.den,cxt->framerate);
    }else if(cxt->codec_type==AVMEDIA_TYPE_AUDIO){
        av_log(NULL,AV_LOG_INFO,"音频 %s 采样率 %d 声道数 %d 采样格式%s\n",
               avcodec_get_name(cxt->codec_id),cxt->sample_rate,cxt->channels,av_get_sample_fmt_name(cxt->sample_fmt));
    }

}
int main(int argc,char* argv[]){
    int resultCode=0;
    av_log_set_level(AV_LOG_INFO);
//    av_register_all();

    if(argc==1){
        av_log(NULL,AV_LOG_WARNING,"usage: media file...");
        return resultCode;
    }

    AVFormatContext *ctx=NULL;

    for(int i=1;i<argc;i++){
        char*url=argv[i];
        //第三个参数自动检测
        resultCode=avformat_open_input(&ctx,url,NULL,NULL);
        if(resultCode<0){
            goto end;
        }
        //第四个参数是 0-输入 1-输出
        av_log(NULL,AV_LOG_WARNING,"input %s\n",url);



        av_log(NULL,AV_LOG_INFO,
               "封装格式：%s 长名字：%s  扩展名 %s\n",
               ctx->iformat->name,ctx->iformat->long_name,
               ctx->iformat->extensions);

        for (int i=0;i<ctx->nb_streams;i++){
            AVStream* stream=ctx->streams[i];
            av_log(NULL,AV_LOG_INFO,
                   "流[%d]: 时间基：%d/%d\n",
                   i,stream->time_base.num,stream->time_base.den);
            printCodec(stream->codec);
        }
//        av_dump_format(ctx,i,url,0);





        for (int j = 0; j <100 ; ++j) {
            AVPacket* pkt=av_packet_alloc();
            av_read_frame(ctx,pkt);

            int streamId=pkt->stream_index;

            if (streamId!=0){
                continue;
            }
            AVRational *time_base = &ctx->streams[streamId]->time_base;

            printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
                   url,
                   av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
                   av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
                   av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
                   pkt->stream_index);

            av_packet_free(&pkt);
        }
        avformat_close_input(&ctx);
        ctx=NULL;
    }


    end:

    if (resultCode){
        av_log(NULL,AV_LOG_ERROR,"%s",av_err2str(resultCode));
    }
    return resultCode;
}
