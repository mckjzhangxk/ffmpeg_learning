
#ifdef __cplusplus
    #ifndef __STDC_CONSTANT_MACROS
    #define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

    #include "stdio.h"
    #include "libavutil/log.h"
    #include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif



//编译命令：
//clang  -I /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/include
//      -L /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/lib
//      -lavutil -lavformat
//      -o extractAudio extractAudio.c

//av_find_best_stream
//av_read_frame,av_packet_unref

//从 流中读取的是 package.
//package 有 1-N个帧 组成

void print_channel_layout_table(){
    printf("声道配置表\n");
    printf("|%10s|%10s|\n","Layout","声道数");
    for (int i = 1; i < 5; ++i) {
        int channels=av_get_channel_layout_nb_channels(i);
        printf("|%10d|%10d|\n",i,channels);
    }
}


// Audio Data Transport Stream  (ADTS)适用于直播
// Audio Data Interchange Format(ADIT)
// https://www.p23.nl/projects/aac-header/

void adts_header(char *szAdtsHeader, int dataLen,int channel_config,int sampling_frequency_index){
    //yoga.movv对应 48K的采用，对应的sampling_frequency_index是3
    //yoga.movv是单通道的
    int audio_object_type = 2;

    int adtsLen = dataLen + 7;

    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit

    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit

    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    szAdtsHeader[6] = 0xfc;
}

// /Users/zhanggxk/Desktop/yoga/yoga.mov /Users/zhanggxk/Desktop/yoga/yoga.aac

//本程序提取的是aac，也就是编码后的音频、
//有2中方式
//1）自己把解封的包 写文件，这里每个包要追加 adts头
//2) 或者借助avcontext写入文件
//3）diff fwrite.aac ffmpeg.aac没有任何区别
int main(int argc, char *argv[]) {

    av_log_set_level(AV_LOG_INFO);
    av_register_all();
    print_channel_layout_table();

    if (argc < 2) {
        av_log(NULL, AV_LOG_WARNING, "usage: extractAudio src\n");
        return -1;
    }
    char *src = argv[1];
    char *dst = "fwrite.aac";
    char *dst1 = "ffmpeg.aac";
    //1.打开源文件，目标文件

    AVFormatContext *ctx = NULL;

    int resultCode = avformat_open_input(&ctx, src, NULL, NULL);
    if (resultCode < 0) {
        return -1;
    }
    //打印基本文件信息，第四个参数是 0-输入 1-输出
    av_dump_format(ctx, 0, src, 0);

    //output acc file
    FILE * dst_fd=fopen(dst,"wb");


//    AVFormatContext *ofctx=NULL;
//    avformat_alloc_output_context2(&ofctx,NULL,NULL,dst1);
    //先分配，再设置 输出格式，等同上面的写法
    AVFormatContext *ofctx=avformat_alloc_context();
    ofctx->oformat= av_guess_format(NULL,dst1,NULL);

    ////////////上下文与输出文件的绑定///////////
    if (!(ofctx->oformat->flags&AVFMT_NOFILE)){
        avio_open(&ofctx->pb,dst1,AVIO_FLAG_WRITE);
    }



    //2.获得要抽取的流
    //对于ts文件，需要调用avformat_find_stream_info,才可以读取到流，这是为什么？
    if((resultCode = avformat_find_stream_info(ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to find stream information: %s, %d(%s)\n",
               src,resultCode,av_err2str(resultCode));
        return -1;
    }
    //第三个参数是需要的 流 索引(ffprobe)，第四个是相关索引
    int audioIndex=av_find_best_stream(ctx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    if(audioIndex<0){
        av_log(ctx,AV_LOG_ERROR,"没有音频流");
        return -1;
    }

    AVStream* stream= avformat_new_stream(ofctx,NULL);
    stream->id=0;
    avcodec_parameters_copy(stream->codecpar,ctx->streams[audioIndex]->codecpar);
    stream->codec->codec_tag=0;
    //3.读取package,输出到文件
    AVPacket packet;
    av_init_packet(&packet);

    avformat_write_header(ofctx,NULL);
    while(av_read_frame(ctx,&packet)>=0){
        if(packet.stream_index==audioIndex){

            char header[7];
            adts_header(header,packet.size,1,3);
            //如果是ts文件，是不用写header的，package.data里面已经包含了header信息
            fwrite(header,1,7,dst_fd);
            fwrite(packet.data,packet.size,1,dst_fd);

            packet.stream_index=0;
            packet.pts= av_rescale_q_rnd(packet.pts,
                                         ctx->streams[audioIndex]->time_base,
                                         stream->time_base,
                                         (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet.duration= av_rescale_q_rnd(packet.duration,
                                         ctx->streams[audioIndex]->time_base,
                                         stream->time_base,
                                         (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet.dts=packet.pts;
            packet.pos=-1;
            av_write_frame(ofctx,&packet);
        }
        //减少引用计数
        av_packet_unref(&packet);
    }

    av_write_trailer(ofctx);


    fclose(dst_fd);
    avformat_close_input(&ctx);
//    avformat_close_input(&ofctx);
    avformat_free_context(ofctx);
    return resultCode;
}
