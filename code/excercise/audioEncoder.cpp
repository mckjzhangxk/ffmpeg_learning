//
// Created by zzzlllll on 2021/12/11.
//

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif


#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include <stdio.h>


#ifdef __cplusplus
}
#endif




void printPackageInfo( AVPacket* pkg,int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %02d,dts %02d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}
//检查是codec的sample_fmts列表有没有sample_fmt
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
//声道数最多的layout,因为一个编码器可以支持很对channel
static int select_channel_layout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}


void createOneFrame(AVFrame* frame,AVCodecContext *c,float &  t){
    //get a empty frame
    float tincr = 1.0f / c->sample_rate;
    uint16_t *samples = (uint16_t*)frame->data[0];
    int j;
    for (j = 0; j < c->frame_size; j++) {
        samples[2*j] = (int)(sin(2 * M_PI * 440.0*t) * 10000);
        int k;
        for (k = 1; k < c->channels; k++)
            samples[2*j + k] = samples[2*j];
        t += tincr;
    }

}
//1-有输出
//0-无输出
//-1 报错

//int encode(AVCodecContext* ctx,AVFrame *frame,AVPacket *packet,FILE * outfile,int steps){
//    int get=0;
//    int result=avcodec_encode_video2(ctx,packet,frame,&get);
//    if(result){
//        av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
//        return -1;
//    }
//    if(get){
//        printPackageInfo(packet,steps);
//        fwrite(packet->data,packet->size,1,outfile);
//        av_packet_unref(packet);
//    }
//
//    return get;
//}
//1-有输出
//0-无输出
//-1 报错
//新的编码函数

int encode(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket, FILE * file) {
    static int iters=0;
    iters++;
    //把frame 输出给编码器
    int ret=avcodec_send_frame(pContext,pFrame);

    if (ret<0){
        return ret;
    }
    while (ret>=0){
        av_init_packet(pPacket);
        //0的时候表示输出成功，还可以继续从编码器取出 编码好的数据
        ret=avcodec_receive_packet(pContext,pPacket);


        if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF){
            //编码器没有足够的数据输出到package
            //不认为这是异常
            return 0;
        } else if (ret<0){
            //编码器异常,ret<0
            return ret;
        }

        //编码器输出到了package中,把数据包写入到文件
//            char header[7];
//            adts_header(header,pPacket->size,pContext->channel_layout,3);
        fwrite(pPacket->data,pPacket->size,1,file);
        printPackageInfo(pPacket,iters);
        av_packet_unref(pPacket);
    }
}
//参考文章
//https://wanggao1990.blog.csdn.net/article/details/115724436
// 查找编码器:avcodec_find_encoder
// 创建编码器上下文，并设置必要参数:avcodec_alloc_context3
// 打开编码器：avcodec_open2

//编码 avcodec_send_packet,avcodec_receive_packet
/*
 * 使用AAC HE2生成frame.aac
 *
 * 重点
 * 1） libfdk_aac的参数设置
 * 2) 编码器对 采样位数,采样率支持的判断
 * */


int main(int argc,char* argv[]){

    avcodec_register_all();

//    AVCodec * p_codecs=avcodec_find_encoder(AV_CODEC_ID_MP2);
    AVCodec * p_codecs=avcodec_find_encoder_by_name("libfdk_aac");

    AVCodecContext*  avCodecContext=avcodec_alloc_context3(p_codecs);


    avCodecContext->sample_fmt=AV_SAMPLE_FMT_S16;  //采用大小
    if (!check_sample_fmt(p_codecs, avCodecContext->sample_fmt)) {

        av_log(NULL,AV_LOG_ERROR, "Encoder does not support sample format %s",av_get_sample_fmt_name(avCodecContext->sample_fmt));
        return -1;
    }

    avCodecContext->sample_rate=select_sample_rate(p_codecs); //码率
    avCodecContext->channel_layout=AV_CH_LAYOUT_STEREO;  //立体声，channels=2
    avCodecContext->channels= av_get_channel_layout_nb_channels(avCodecContext->channel_layout);
//    avCodecContext->bit_rate=64000;
    avCodecContext->bit_rate=0;  //如果要profile生效，需要把bit_rate置0.
    avCodecContext->profile=FF_PROFILE_AAC_HE_V2; //lc:128k,he:64k

    int result=avcodec_open2(avCodecContext,p_codecs,NULL);
    if(result){
        av_log(NULL,AV_LOG_ERROR,"can not open context:%s",av_err2str(result));
        return -1;
    }

    AVPacket* packet=av_packet_alloc();
    FILE *outfile=fopen("frame.aac","wb");



    AVFrame* frame=av_frame_alloc();
    frame->format=avCodecContext->sample_fmt;
    frame->channel_layout=avCodecContext->channel_layout;
//    frame->channels=avCodecContext->channels;
    frame->nb_samples=avCodecContext->frame_size;//单通道 的采样数

    //有多重方法分配空间
    result=av_frame_get_buffer(frame,0);
    av_log(NULL,AV_LOG_INFO,"Alloc  FCM  %ld bytes\n",result);
    if(result<0){
        av_log(NULL,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
        return -1;
    }
    int i;
    float t;
    for(i=0;i<200;i++){
        av_init_packet(packet);

        av_frame_make_writable(frame);
        //get a empty frame
        createOneFrame(frame,avCodecContext,t);
        frame->pts=i;

        if(encode(avCodecContext,frame,packet,outfile)<0){
            return -1;
        }
    }



    encode(avCodecContext,NULL,packet,outfile);

    fclose(outfile);

    avcodec_free_context(&avCodecContext);
    av_frame_free(&frame);
}