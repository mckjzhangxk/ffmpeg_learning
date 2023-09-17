#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/avutil.h"
    #include "libavutil/opt.h"
    #include "libavutil/timestamp.h"
#include <stdio.h>


#define FPS 15

#ifdef __cplusplus
}
#endif





void printPackageInfo( AVPacket* pkg,int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %04d,dts %04d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}

int encode(AVCodecContext* ctx,AVPacket *packet,AVFrame *frame,AVFormatContext* out_ctx){
//    static int steps=0;

    int result=avcodec_send_frame(ctx,frame);
    if (result<0){
        av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
        return -1;
    }

    //while循环是因为编码器 可能会吐出多个包

    AVStream *newStram=out_ctx->streams[0];
    AVRational time_base=(AVRational){1,FPS};
    while (result>=0){
        av_init_packet(packet);
        result= avcodec_receive_packet(ctx,packet);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            av_packet_unref(packet);
            return 0;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
            av_packet_unref(packet);
            return -1;
        }
        packet->duration=1;
        packet->pts=av_rescale_q_rnd(packet->pts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet->dts=av_rescale_q_rnd(packet->dts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet->duration=av_rescale_q_rnd(packet->duration,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));


        av_write_frame(out_ctx,packet);
        av_packet_unref(packet);
    }
}

#define  WIDTH 928
#define  HEIGHT 480
#define INFILE "test1.h264"
#define OUTFILE "test1/playlist.m3u8"
#define OUT_TS_FILE "test1/test1-%05d-00.ts"

int main(int argc,char* argv[]){

    avcodec_register_all();

    AVFormatContext *ifmt_ctx = NULL;
    char* in_filename=INFILE;
    const char* out_file=OUTFILE;

    int ret=0;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return -1;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);



    ///////////////这里创建编码器的意义在于 用于指定流信息
    AVCodec * p_codecs=avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext*  avCodecContext=avcodec_alloc_context3(p_codecs);

    avCodecContext->time_base=(AVRational){1,FPS};
    avCodecContext->framerate=(AVRational){FPS,1};
    avCodecContext->width=WIDTH;
    avCodecContext->height=HEIGHT;
    avCodecContext->pix_fmt=AV_PIX_FMT_YUV420P;
    avCodecContext->bit_rate=400000;

//    avCodecContext->gop_size=10;
//    avCodecContext->max_b_frames=1;
    //针对H264
//    av_opt_set(avCodecContext->priv_data,"preset","slow",0);

    int result=avcodec_open2(avCodecContext,p_codecs,NULL);
    if(result){
        av_log(NULL,AV_LOG_ERROR,"can not open context:%s",av_err2str(result));
        exit(1);
    }

    ////////////////////////////创建解码器上下文////////////////////////////
    p_codecs = avcodec_find_decoder(AV_CODEC_ID_H264);

    AVCodecContext *decode_avCodecContext = avcodec_alloc_context3(p_codecs);

    ////////////////////////////编码器参数设置////////////////////////////
    decode_avCodecContext->time_base=(AVRational){1,FPS};
    decode_avCodecContext->framerate=(AVRational){FPS,1};
    decode_avCodecContext->width=WIDTH;
    decode_avCodecContext->height=HEIGHT;
    decode_avCodecContext->pix_fmt=AV_PIX_FMT_YUV420P;
    decode_avCodecContext->bit_rate=400000;

    result = avcodec_open2(decode_avCodecContext, p_codecs, NULL);
    if (result) {
        av_log(NULL, AV_LOG_ERROR, "can not open context:%s", av_err2str(result));
        return -1;
    }

    AVFormatContext* ofmctx=NULL;
    avformat_alloc_output_context2(&ofmctx,NULL,NULL,out_file);
    if(!ofmctx){
        av_log(NULL,AV_LOG_ERROR,"can't create out context");
        return -1;
    }
    AVFrame *frame = av_frame_alloc();

    //创建流信息
//    我怎么样才可以设置 流编解码信息？
    AVStream * newStram=avformat_new_stream(ofmctx,p_codecs);
    avcodec_parameters_from_context(newStram->codecpar,avCodecContext);
    newStram->codecpar->codec_tag = 0;


    av_opt_set_int(ofmctx->priv_data,"hls_time", 5, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(ofmctx->priv_data,"hls_list_size", 5, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(ofmctx->priv_data,"hls_segment_filename", OUT_TS_FILE, AV_OPT_SEARCH_CHILDREN);



    av_dump_format(ofmctx,0,out_file,1);


    int noNeedProvideFile=ofmctx->flags&AVFMT_NOFILE;
    if( !noNeedProvideFile){
        avio_open(&ofmctx->pb,out_file,AVIO_FLAG_WRITE);
    }


    result=avformat_write_header(ofmctx,NULL);


    if(result){
        av_log(NULL,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
        return -1;
    }



    AVPacket packet;
    av_init_packet(&packet);
    packet.data=NULL;
    packet.size=0;


    AVPacket new_packet;
    av_init_packet(&new_packet);
    new_packet.data=NULL;
    new_packet.size=0;

    int pos=0;
    while (1){
        ret= av_read_frame(ifmt_ctx,&packet);
        if (ret<0){
            break;
        }


        int result=avcodec_send_packet(decode_avCodecContext,&packet);
        if (result<0){
            av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
            return -1;
        }

        //while循环是因为编码器 可能会吐出多个包
        while (result>=0){
            result=avcodec_receive_frame(decode_avCodecContext,frame);
            if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
                //编码器没有足够的数据进行输出到package
                break;
            }else if(result<0){
                // codec not opened, or it is an encoder other errors: legitimate decoding errors
                //编码器其他异常
                av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
                return -1;
            }
            //正常输出
            //输出帧信息
            frame->pts=pos++;
            encode(avCodecContext,&new_packet,frame,ofmctx);
        }


        av_packet_unref(&packet);
        av_packet_unref(&new_packet);

    }


    result=avcodec_send_packet(decode_avCodecContext,NULL);
    if (result<0){
        av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
        return -1;
    }
    while (result>=0){
        result=avcodec_receive_frame(decode_avCodecContext,frame);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            break;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
            return -1;
        }
        //正常输出
        //输出帧信息
        frame->pts=pos++;
        encode(avCodecContext,&new_packet,frame,ofmctx);
    }

    encode(avCodecContext,&new_packet,NULL,ofmctx);

    av_write_trailer(ofmctx);
    avcodec_free_context(&avCodecContext);
    avformat_free_context(ofmctx);
}