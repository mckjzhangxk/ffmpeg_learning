//
// Created by zzzlllll on 2021/12/11.
//

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include <stdio.h>


#ifdef __cplusplus
}
#endif


#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

static void saveBMP(struct SwsContext *swsCtx,
                    AVFrame *frame,
                    int w,
                    int h,
                    char *name){
    FILE *f = NULL;
    int dataSize = w * h * 3;

    //1. 先进行转换，将YUV frame 转成  BGR24 Frame
    AVFrame *frameBGR= av_frame_alloc();
    frameBGR->width = w;
    frameBGR->height = h;
    frameBGR->format = AV_PIX_FMT_BGR24;

    av_frame_get_buffer(frameBGR, 0);

    sws_scale(swsCtx,
              (const uint8_t * const *)frame->data,
              frame->linesize,
              0,
              frame->height,
              frameBGR->data,
              frameBGR->linesize);
    //2. 构造 BITMAPINFOHEADER
    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = w;
    infoHeader.biHeight = h * (-1);
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = 0;
    infoHeader.biClrImportant = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biPlanes = 1;

    //3. 构造 BITMAPFILEHEADER
    BITMAPFILEHEADER fileHeader = {0, };
    fileHeader.bfType = 0x4d42; //'BM'
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataSize;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //4. 将数据写到文件
    f = fopen(name, "wb");
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, f);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, f);
    fwrite(frameBGR->data[0], 1, dataSize, f);

    //5. 释放资源
    fclose(f);
    av_freep(&frameBGR->data[0]);
    av_free(frameBGR);
}
void save_gray_pic(char filename[255], AVFrame *pFrame);

void printPackageInfo(AVPacket* pkg, int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %02d,dts %02d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}




//0-正常,1 报错
//使用新的编码函数
int decode(AVCodecContext* ctx,AVFrame *frame,AVPacket *packet,char *dst){
    static int steps;

    int result=avcodec_send_packet(ctx,packet);
    if (result<0){
        av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
        return -1;
    }

    //while循环是因为编码器 可能会吐出多个包
    while (result>=0){
        result=avcodec_receive_frame(ctx,frame);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            return 0;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
            return -1;
        }
        //正常输出
        printPackageInfo(packet,steps);
        char filename[255];
        sprintf(filename,"%s%d.bmp",dst,steps++);
        save_gray_pic(filename,frame);
    }
}

void save_gray_pic(char filename[255], AVFrame *pFrame) {
    FILE * f=fopen(filename,"wb");
    int line_size=pFrame->linesize[0];
    uint8_t *start_buffer=pFrame->data[0];


    fprintf(f, "P5\n%d %d\n%d\n", pFrame->width, pFrame->height, 255);
    for (int i = 0; i < pFrame->height; ++i) {
        uint8_t* data=start_buffer+i*line_size;

        fwrite(data,pFrame->width,1,f);
    }
    fclose(f);
}





//编码器输出的nalu头部会加入0000000001的开始码，所以播放器可以正常播放frame.h264
int main(int argc,char* argv[]) {
    if (argc < 3) {
        av_log(NULL, AV_LOG_WARNING, "usage: extractVideo src dst\n");
        return -1;
    }
    char *src = argv[1];
    char *dst = argv[2];


    avcodec_register_all();
    AVFormatContext *ctx = NULL;

    int resultCode = avformat_open_input(&ctx, src, NULL, NULL);

    if ((resultCode = avformat_find_stream_info(ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to find stream information: %s, %d(%s)\n",
               src, resultCode, av_err2str(resultCode));
        return -1;
    }
    //第三个参数是需要的 流 索引(ffprobe)，第四个是相关索引
    int videoIndex = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);


    //打印基本文件信息，第四个参数是 0-输入 1-输出


    ////////////////////////////创建解码器上下文////////////////////////////
    AVCodec *p_codecs = avcodec_find_decoder(ctx->streams[videoIndex]->codec->codec_id);
    p_codecs=avcodec_find_decoder(AV_CODEC_ID_HEVC);
    AVCodecContext *avCodecContext = avcodec_alloc_context3(p_codecs);

    ////////////////////////////编码器参数设置////////////////////////////

    avcodec_parameters_to_context(avCodecContext, ctx->streams[videoIndex]->codecpar);


    int result = avcodec_open2(avCodecContext, p_codecs, NULL);
    if (result) {
        av_log(NULL, AV_LOG_ERROR, "can not open context:%s", av_err2str(result));
        return -1;
    }


    AVFrame *frame = av_frame_alloc();


    while (1) {
        AVPacket *packet = av_packet_alloc();

        int ret = av_read_frame(ctx, packet);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_read_frame error :%s", av_err2str(result));
            return -1;
        }

        decode(avCodecContext, frame, packet, dst);
        av_free_packet(packet);
    }
    decode(avCodecContext, frame, NULL, dst);
}