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
#include <stdio.h>
#define  FRAME_WIDTH 640
#define  FRAME_HEIGHT 480
#define FPS 30

#ifdef __cplusplus
}
#endif





void printPackageInfo( AVPacket* pkg,int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %02d,dts %02d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}

void createOneFrame(AVFrame* frame,int  i){
    //get a empty frame
    //y
    int r,c;
    for(r=0;r<FRAME_HEIGHT;r++){
        for (c=0;c<FRAME_WIDTH;c++){
            int y=c + r + i * 3;
            int index=r*frame->linesize[0]+c;
            frame->data[0][index]=y; //Y
        }
    }
    for ( r = 0; r <FRAME_HEIGHT/2 ; ++r) {
        for ( c = 0; c < FRAME_WIDTH/2; ++c) {
            int u=128 +  r+i * 2;
            int v=64 + c  +i* 5;;

            int index=r*frame->linesize[1]+c;
            frame->data[1][index]=u;

            index=r*frame->linesize[2]+c;
            frame->data[2][index]=v;
        }
    }

}

int main(int argc,char* argv[]){

    avcodec_register_all();

    AVCodec * p_codecs=avcodec_find_encoder(AV_CODEC_ID_H264);
//    AVCodec * p_codecs=avcodec_find_encoder_by_name("libx264");

    AVCodecContext*  avCodecContext=avcodec_alloc_context3(p_codecs);

    avCodecContext->time_base=(AVRational){1,FPS};
    avCodecContext->framerate=(AVRational){FPS,1};
    avCodecContext->width=FRAME_WIDTH;
    avCodecContext->height=FRAME_HEIGHT;
    avCodecContext->pix_fmt=AV_PIX_FMT_YUV420P;
    avCodecContext->bit_rate=400000;


    avCodecContext->gop_size=10;
    avCodecContext->max_b_frames=1;




    //针对H264
    av_opt_set(avCodecContext->priv_data,"preset","slow",0);

    //这一句到底怎么意思？
    int result=avcodec_open2(avCodecContext,p_codecs,NULL);
    if(result){
        av_log(NULL,AV_LOG_ERROR,"can not open context:%s",av_err2str(result));
        exit(1);
    }

    AVPacket* packet=av_packet_alloc();


    int got=0;

    const char* out_file="ts/frame.m3u8";
    AVFormatContext* ofmctx=NULL;
    avformat_alloc_output_context2(&ofmctx,NULL,NULL,out_file);
    if(!ofmctx){
        av_log(NULL,AV_LOG_ERROR,"can't create out context");
        return -1;
    }
    //创建流信息
//    我怎么样才可以设置 流编解码信息？
    AVStream * newStram=avformat_new_stream(ofmctx,p_codecs);
//    avcodec_parameters_copy(newStram->codecpar,p_codecs.)

    avcodec_parameters_from_context(newStram->codecpar,avCodecContext);
    newStram->codecpar->codec_tag = 0;
//    newStram->time_base=avCodecContext->time_base;
    av_dump_format(ofmctx,0,out_file,1);


    int noNeedProvideFile=ofmctx->flags&AVFMT_NOFILE;
    if( !noNeedProvideFile){
        avio_open(&ofmctx->pb,out_file,AVIO_FLAG_WRITE);
    }


    AVFrame* frame=av_frame_alloc();
    frame->width=avCodecContext->width;
    frame->height=avCodecContext->height;
    frame->format=avCodecContext->pix_fmt;




    result=av_frame_get_buffer(frame,0);
    if(result){
        av_log(NULL,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
        return -1;
    }
    int cnt=0,i;
//    newStram->time_base.num=1;
//    newStram->time_base.den=1;

    result=avformat_write_header(ofmctx,NULL);


    if(result){
        av_log(NULL,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
        return -1;
    }

    AVRational time_base=(AVRational){1,FPS};
    for(i=0;i<150;i++,cnt++){
        av_init_packet(packet);
        packet->data=NULL;
        packet->size=0;

        av_frame_make_writable(frame);
        //get a empty frame
        createOneFrame(frame,i);
        frame->pts=i;
        //
        result=avcodec_encode_video2(avCodecContext,packet,frame,&got);
        if(result){
            av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
            exit(1);
        }
        if(got){
            packet->pts=av_rescale_q_rnd(packet->pts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet->dts=av_rescale_q_rnd(packet->dts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet->duration=av_rescale_q_rnd(packet->duration,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));


            printPackageInfo(packet,cnt);
//            fwrite(packet->data,packet->size,1,outfile);
            av_write_frame(ofmctx,packet);
            av_packet_unref(packet);
        }

    }

    got=1;
    while (got){
        cnt++;
        result=avcodec_encode_video2(avCodecContext,packet,NULL,&got);
        if(got){
            packet->pts=av_rescale_q_rnd(packet->pts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet->dts=av_rescale_q_rnd(packet->dts,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet->duration=av_rescale_q_rnd(packet->duration,time_base,newStram->time_base,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

            printPackageInfo(packet,cnt);
//            fwrite(packet->data,packet->size,1,outfile);
            av_write_frame(ofmctx,packet);
            av_packet_unref(packet);
        }
    }


    av_write_trailer(ofmctx);

    avcodec_free_context(&avCodecContext);
    av_frame_free(&frame);
    avformat_free_context(ofmctx);
}