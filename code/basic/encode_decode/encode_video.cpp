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


#define  FRAME_WIDTH  2940
#define  FRAME_HEIGHT 1912
#define  FPS 20
#define  SECOND 1


void printPackageInfo(AVPacket* pkg,int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %02d,dts %02d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}

FILE * f=NULL;
//文件的存储是YUV,我们要确保frame->line[0]==width,frame->line[1]=frame->line[2]==width//2
//这样才不会出错，如果linesize>width,一次读取w*h,读取会错位。
void createOneFrameFromFile(AVFrame* frame,int  i){
    if (!f){
        f= fopen("video.yuv","rb");
    }
    //YYYYYYYY UUVV
    int ysize=frame->width*frame->height;
    int usize=ysize/4;
    int vsize=ysize/4;

    int result=0;


    result=fread(frame->data[0],ysize,1,f);
    if (result<0){
        av_log(frame,AV_LOG_ERROR,"读取文件失败");
        exit(1);
    }
    result=fread(frame->data[1],usize,1,f);
    if (result<0){
        av_log(frame,AV_LOG_ERROR,"读取文件失败");
        exit(1);
    }
    result=fread(frame->data[2],vsize,1,f);
    if (result<0){
        av_log(frame,AV_LOG_ERROR,"读取文件失败");
        exit(1);
    }

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


//0-正常,1 报错
//使用新的编码函数,avcodec_send_frame,avcodec_receive_packet
//encode可以理解成以下流程：
//1)向编码器 发送一帧 frame
//2)从编码器 中输出尽可能多的package，写入到outfile
int encode(AVCodecContext* ctx,AVFrame *frame,AVPacket *packet,FILE * outfile){
    static int steps=0;
    int result=avcodec_send_frame(ctx,frame);
    if (result<0){
        av_log(ctx,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
        return -1;
    }

    //while循环是因为编码器 可能会吐出多个包
    while (result>=0){
        av_init_packet(packet);
        result=avcodec_receive_packet(ctx,packet);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            return 0;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(ctx,AV_LOG_ERROR,"%s", av_err2str(result));
            return -1;
        }
        //正常输出
        printPackageInfo(packet,++steps);
        fwrite(packet->data,packet->size,1,outfile);
        av_packet_unref(packet);
    }
}

//参考文章
//https://wanggao1990.blog.csdn.net/article/details/115724436

/*
 *
 * 使用libx264编码 自定义的视频，生成文件frame.h264。
 * 0.输出文件定义
 * 1.创建编码器
 * 2.创建编码器上下文
 * 3.设置编码器上下文参数
 * 4.绑定【编码器】与【编码器上下文】
 * 5.创建AVFrame,AVPackage
 * 6.分配AVFrame空间，生成内容,createOneFrame
 *   6.1 分配frame时候，需要关注align参数的设置！！！
 * 7.编码，输出
 * 重点
 * 1） codec context关于 h264的参数设置
 * 2)  生成的frame.h264视频的fps受我们设置的FPS参数控制，而我在之前的测试中，这个参数
 * 控制好像没生效，为什么？
 * 答案：在SPS中，如果设置了vui的num_units_in_tick，time_scale，FPS=time_scale/num_units_in_tick/2
 * 否则ffplay会按照25fps播放视频。
 *
 * 这也是我之前很困惑的地方，因为tvt生成的h264都是没有设置这2个参数的，所以ffplay会25fps播放视频。但是把tvt.h264如果转封装，
 * 则会转换失败或者播放刷屏
 *

 * */

//编码器输出的nalu头部会加入0000000001的开始码，所以播放器可以正常播放frame.h264
//由于是裸数据h264,ffplay会默认使用25 fps来播放视频
//程序的输出是否正确，关键要看是否能正确输出FPS*SECOND 帧数据
int main(int argc,char* argv[]){

    avcodec_register_all();

    ////////////////////////////创建编码器上下文////////////////////////////
    //AVCodec * p_codecs=avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodec * p_codecs=avcodec_find_encoder_by_name("libx264");

    AVCodecContext*  avCodecContext=avcodec_alloc_context3(p_codecs);

    ////////////////////////////编码器参数设置////////////////////////////
    //sps相关参数
    avCodecContext->profile=FF_PROFILE_H264_HIGH_444; //高规格的算法
    avCodecContext->level=50;  //视频等级，向下兼容


    avCodecContext->width=FRAME_WIDTH; //sps
    avCodecContext->height=FRAME_HEIGHT;//sps
    avCodecContext->pix_fmt=AV_PIX_FMT_YUV420P;


    //甚至帧率
    avCodecContext->time_base=(AVRational){1,FPS};//帧与帧之间间隔的时间
    avCodecContext->framerate=(AVRational){FPS,1}; //
    //设置码率
    avCodecContext->bit_rate=600000; //600kps


    //GOP相关参数
    //GOP过大，丢包会等很长时间恢复，过小压缩率又太低
    avCodecContext->gop_size=50;    //gop大小
    avCodecContext->keyint_min=10;  //gop最小可以keyint_min由个帧组成

    //B帧
    avCodecContext->has_b_frames=1;
    avCodecContext->max_b_frames=3; //2个P帧最多max_b_frames个B帧

    //参考帧，通俗的说就是一个p帧 可以参考 前面几个帧 来解码
    avCodecContext->refs=1;
    //针对H264
    if (p_codecs->id==AV_CODEC_ID_H264){
        //h264的预设参数,slow的压缩 视频质量高
        av_opt_set(avCodecContext->priv_data,"preset","slow",0);
    }

    int result=avcodec_open2(avCodecContext,p_codecs,NULL);
    if(result){
        av_log(avCodecContext,AV_LOG_ERROR,"can not open context:%s",av_err2str(result));
        return -1;
    }

    AVPacket packet;
    FILE *outfile=fopen("frame.h264","wb");



    AVFrame* frame=av_frame_alloc();
    frame->width=avCodecContext->width;
    frame->height=avCodecContext->height;
    frame->format=avCodecContext->pix_fmt;

    //手动分配buffer
//    int Size=FRAME_WIDTH*FRAME_HEIGHT;
//    frame->data[0]=(uint8_t *)malloc(8*Size);
//    frame->data[1]=(uint8_t *)malloc(2*Size);
//    frame->data[2]=(uint8_t *)malloc(2*Size);
//    frame->linesize[0]=FRAME_WIDTH;
//    frame->linesize[1]=FRAME_WIDTH/2;
//    frame->linesize[2]=FRAME_WIDTH/2;

//    result=av_image_alloc(&frame->data,frame->linesize,frame->width,frame->height,frame->format,1);
    //有多重方法分配空间
    //align=k,表示设置的linesize%k==0,如果设置为2，4，有可能导致 linesize[0]>witdh,linesize[1]>width/2
    //k=0表示系统会自动选取合适的k.
    result=av_frame_get_buffer(frame,1);
    if(result<0){
        av_log(frame,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
        return -1;
    }
    //frame->data[0],data[1],data[2]分别对应Y,U,V三个通道。
    //frame->line[0],frame->line[1],frame->line[1]分别是width,width/2,width/2，也就是一行多少数据
    //总大小应该是 width*height*1.5

    int size=frame->linesize[0]*frame->height
            +frame->linesize[1]*frame->height/2
            +frame->linesize[2]*frame->height/2;
    int size_cal=frame->width*frame->height
            +frame->width*frame->height/4
            +frame->width*frame->height/4;
    av_log(NULL,AV_LOG_INFO,"分配 %d x %d 的YUV420 %ld 字节，理论值为 %ld\n",
           frame->width,frame->height,size,size_cal);

    for(int i=0;i<SECOND*FPS;i++){
        //如果frame->data被锁定，分配buf,把现在的data搬运到buf中，以避免同时对data的操作，造成的脏数据
        av_frame_make_writable(frame);
        //get a empty frame
//        createOneFrame(frame,i);
        createOneFrameFromFile(frame,i);
        frame->pts=i;

        if(encode(avCodecContext,frame,&packet,outfile)<0){
            av_log(NULL,AV_LOG_ERROR,"编码异常",NULL);
            return -1;
        }
    }

    encode(avCodecContext,NULL,&packet,outfile);


    //文件tail
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    fwrite(endcode,sizeof(endcode),1,outfile);
    fclose(outfile);

    avcodec_free_context(&avCodecContext);
    av_frame_free(&frame);

    if (f){
        fclose(f);
    }
}

