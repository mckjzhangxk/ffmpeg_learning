#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#ifdef __cplusplus
}
#endif
void adts_header(char *szAdtsHeader, int dataLen,int channel_config,int sampling_frequency_index){
//我把sampling_frequency_index.channel_config改成 3,1后可以正常播放，
//请理解这两个参数的含义！
    //yoga.movv对应 48K的采用，对应的sampling_frequency_index是3
    //yoga.movv是单通道的
    int audio_object_type = 2; //lc


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

void printPackageInfo( AVPacket* pkg,int iters){
    av_log(NULL,AV_LOG_INFO,"iter %d: pts %02d,dts %02d,size %d,duration=%ld\n",iters,pkg->pts,pkg->dts,pkg->size,pkg->duration);
}
int getSampleSize(AVSampleFormat format){
//    return av_get_bytes_per_sample(format);
    switch (format) {
        case  AV_SAMPLE_FMT_U8:
            return 1;
        case AV_SAMPLE_FMT_S16:
            return 2;
        case AV_SAMPLE_FMT_S32:
            return 4;
        case AV_SAMPLE_FMT_FLT:
            return 4;
    }
    return 0;
}

class Converter{
public:
    Converter(int packagesize,AVCodecContext* in_code_ctx,AVCodecContext* out_code_ctx){
        //设置转换规则
        m_swrContext=swr_alloc_set_opts(NULL,
                                        out_code_ctx->channel_layout,
                                        out_code_ctx->sample_fmt,
                                        out_code_ctx->sample_rate,
                                        in_code_ctx->channel_layout,
                                        in_code_ctx->sample_fmt,
                                        in_code_ctx->sample_rate,
                                        0, NULL
        );
        if(!m_swrContext){
            av_log(NULL,AV_LOG_ERROR,"无法创建重采样上下文");
        }

        //packagesize=sample_size*channel*samples
        in_samples=packagesize/(in_code_ctx->channels* getSampleSize(in_code_ctx->sample_fmt));
        //参数是uint8_t ***的原因是 不同channel数据(planar)可能独立存放。
        //一个channel的数据用uint8_t *存储，多个channel就需要uint8_t **的表，
        //给这张表赋值，就是uint8_t ***。

        //但实际中，读取的package 都不是planar的,我们只会使用in_data[0],out_data[0]来保存数据
        av_samples_alloc_array_and_samples(
                &in_data,     //缓冲区
                &in_line_size,    //缓冲区大小
                in_code_ctx->channels,  //声道数量
                in_samples, //每个通道的样本点
                in_code_ctx->sample_fmt,        //数据格式 如f32le
                0);
        out_samples=(in_samples*out_code_ctx->sample_rate)/in_code_ctx->sample_rate;

        av_samples_alloc_array_and_samples(
                &out_data,     //缓冲区
                &out_line_size,    //缓冲区大小
                out_code_ctx->channels,  //声道数量
                out_samples, //每个通道的样本点
                out_code_ctx->sample_fmt,        //数据格式 如f32le
                0);
    }

    void convert(AVPacket * packet){
        memcpy((void *)in_data[0],(void *)packet->data,packet->size);
        swr_convert(m_swrContext,
                    out_data,                    //重采样后的数据
                    out_samples,           //输出样本端/channel
                    (const uint8_t **)(in_data),  //原数据
                    in_samples);            //输入样本点/channel
        packet->data= reinterpret_cast<uint8_t *>(out_data);
        packet->size=out_line_size;
    }
    virtual ~Converter(){
        av_freep(in_data[0]);
        av_freep(out_data[0]);
        av_freep(in_data);
        av_freep(out_data);
        swr_free(&m_swrContext);
    }
private:
    SwrContext *m_swrContext;
    uint8_t **in_data;
    uint8_t **out_data;
    int in_line_size;
    int out_line_size;

    int in_samples;
    int out_samples;
};

class YUVWriter{
public:
    YUVWriter(const char *prefix){
        char filename[256];
        sprintf(filename,"%s.aac",prefix);
        fp= fopen(filename,"wb");
    }
    bool WritePackage(AVPacket* packet){
        return fwrite(packet->data,packet->size,1,fp)>0;
    }
    bool WritePackage(uint8_t* data,int size){
        return fwrite(data,size,1,fp)>0;
    }
    bool WritePackage(AVPacket* packet, char header[7]){
        fwrite(header,7,1,fp);
        return fwrite(packet->data,packet->size,1,fp)>0;
    }
    virtual ~YUVWriter(){
        fclose(fp);
    }
private:
    FILE *fp;
};


static int encode(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket, YUVWriter * writer);

void  print_codec_info(const char *title, AVCodecContext * in_codes_ctx){
    AVCodec * codecs=avcodec_find_encoder(in_codes_ctx->codec_id);

    av_log(NULL, AV_LOG_WARNING, "\t%s\n编码器 %s,%s,\n声道数 %d,sample rate %d,位深 %dbits,比特率 %ld\n",
           title,
           codecs->name,
           codecs->long_name,
           in_codes_ctx->channels,
           in_codes_ctx->sample_rate,
           getSampleSize(in_codes_ctx->sample_fmt),
           in_codes_ctx->bit_rate
    );
}
//播放 pcm
// ffplay -ar 48000 -f f32le -ac 1 voice.pcm

//演示重采样
// 重采样是对 原始数据的处理
// raw data(ar1,size1,ch1)---> swr_convert --> raw data(ar2,size2,ch2)

//问题，加入adts后无法播放，查看问题？
int main(){
    char errbuf[128]={0};
    av_log_set_level(AV_LOG_INFO);
    ////////////////////////////注册所有设备////////////////////////////
    avdevice_register_all();

    ////////////////////////////设置采集方式////////////////////////////
    const char *driverName="avfoundation";
    AVInputFormat * inputFormat=av_find_input_format(driverName);
    if (!inputFormat){
        av_log(NULL,AV_LOG_ERROR,"不支持本格式:%s",driverName);
        return -1;
    }



    ////////////////////////////打开设备////////////////////////////
    AVFormatContext* cxt=NULL;
    //mac下的格式是[videoId:audioId]
    const char *deviceName=":0";
    int result=avformat_open_input(&cxt,deviceName,inputFormat,NULL);
    if (result<0){
        av_strerror(result,errbuf,128);
        av_log(NULL,AV_LOG_ERROR,"打开输入上下文异常 %s",errbuf);
        return -1;
    }
    //进一步获得留信息
    if(avformat_find_stream_info(cxt,NULL)<0){
        av_strerror(result,errbuf,128);
        av_log(NULL,AV_LOG_ERROR,"获得流信息异常 %s",errbuf);
        return -1;
    }

    ////////////////////////////输出流信息////////////////////////////////////////////
    av_log(NULL,AV_LOG_INFO,"设备打开成功\n");
    AVCodecContext * in_codes_ctx=cxt->streams[0]->codec;
    print_codec_info("输入流",in_codes_ctx);



    ////////////////////////////重采用参数，转换设置////////////////////////////////////////////
    //设置重采样codec_context
    AVCodecContext* raw_codes_cxt= avcodec_alloc_context3(NULL);
    raw_codes_cxt->codec_id=AV_CODEC_ID_PCM_S16LE;
    raw_codes_cxt->sample_fmt=AV_SAMPLE_FMT_S16;

    raw_codes_cxt->channel_layout=AV_CH_LAYOUT_STEREO;//立体声
    raw_codes_cxt->channels= av_get_channel_layout_nb_channels(raw_codes_cxt->channel_layout);
    raw_codes_cxt->sample_rate=in_codes_ctx->sample_rate;

    print_codec_info("重采样输出",raw_codes_cxt);
    //具体的转换规则
    SwrContext * swrContext=swr_alloc_set_opts(NULL,
                                  raw_codes_cxt->channel_layout,
                                  raw_codes_cxt->sample_fmt,
                                  raw_codes_cxt->sample_rate,
                                  in_codes_ctx->channel_layout,
                                  in_codes_ctx->sample_fmt,
                                  in_codes_ctx->sample_rate,
                                  0, NULL
                       );
    if(!swrContext){
        av_log(NULL,AV_LOG_ERROR,"无法创建重采样上下文");
        return -1;
    }

    if(swr_init(swrContext)<0){
        av_log(NULL,AV_LOG_ERROR,"无法初始化重采样");
        return -1;
    }

    //重采样操作 的输入输出缓冲设置
    int raw_data_size,in_data_size;
    uint8_t ** raw_data,**in_data;

    //输入缓冲区的设置
    //单通道一个音频帧的样本数 ,2048是一个读入的package消息
    int in_samples_per_channel= 2048 / getSampleSize(in_codes_ctx->sample_fmt) / in_codes_ctx->channels;
    av_samples_alloc_array_and_samples(
            &in_data,     //缓冲区
            &in_data_size,    //缓冲区大小
            in_codes_ctx->channels,  //声道数量
            in_samples_per_channel, //每个通道的样本点
            AV_SAMPLE_FMT_FLT,        //数据格式 如f32le
            0);

    //输出缓冲区的设置
    int out_samples_per_channel=in_samples_per_channel;
    av_samples_alloc_array_and_samples(
            &raw_data,     //缓冲区
            &raw_data_size,    //缓冲区大小
            raw_codes_cxt->channels,  //声道数量
            out_samples_per_channel, //每个通道的样本点
            raw_codes_cxt->sample_fmt,        //数据格式 如f32le
            0);

    ////////////////////////////编码器的设置////////////////////////////////////////////
    AVCodec * out_codecs= avcodec_find_encoder_by_name("libfdk_aac");
    AVCodecContext* out_codecs_cxt= avcodec_alloc_context3(out_codecs);
    //参数设置
    out_codecs_cxt->channel_layout=raw_codes_cxt->channel_layout;
    out_codecs_cxt->channels=raw_codes_cxt->channels;
    out_codecs_cxt->sample_fmt=raw_codes_cxt->sample_fmt;
    out_codecs_cxt->sample_rate=raw_codes_cxt->sample_rate;
    out_codecs_cxt->profile=FF_PROFILE_AAC_LOW;
    out_codecs_cxt->bit_rate=0;
    print_codec_info("编码器输出",out_codecs_cxt);
    result=avcodec_open2(out_codecs_cxt,out_codecs,NULL);
    if(result<0){
        av_log(NULL,AV_LOG_ERROR,"打开编码器失败,%s",av_err2str( result));
        return -1;
    }
    //原始输出帧
    AVFrame* out_frame=av_frame_alloc();
    out_frame->channel_layout= out_codecs_cxt->channel_layout;
    out_frame->channels= av_get_channel_layout_nb_channels(out_frame->channel_layout);
    out_frame->format=out_codecs_cxt->sample_fmt;
    out_frame->nb_samples=out_samples_per_channel; //单声道输出的样本数量
    result=av_frame_get_buffer(out_frame,0);

    if(result){
        av_log(NULL,AV_LOG_ERROR,"分配输出帧buffer异常,%s",av_err2str( result));
        return -1;
    }

    //编码后的输出包
    AVPacket out_packet;

    ////////////////////////////读取设备数据////////////////////////////
    //4.读取设备包
    YUVWriter pcmWriter("voice");
    AVPacket in_packet;
    av_init_packet(&in_packet);//除了data,size的字段的初始化
    //录制300个包
    for (int i = 0; i < 300; ++i) {
        if (av_read_frame(cxt,&in_packet) < 0){
            av_log(NULL,AV_LOG_ERROR,"读取包失败");
            break;
        }
        //重采样代码
        memcpy((void *)in_data[0], (void *)in_packet.data, in_packet.size);
        swr_convert(swrContext,
                    raw_data,                    //采样后的数据
                    out_samples_per_channel,           //【输出】单通道一个音频帧的样本数
                    (const uint8_t **)(in_data),  //原数据
                    in_samples_per_channel);            //【输入】单通道一个音频帧的样本数

//        in_packet.data=raw_data[0];
//        in_packet.size=raw_data_size;
//        pcmWriter.WritePackage(&in_packet);
        //////////////////编码的代码
        memcpy(out_frame->data[0], raw_data[0], raw_data_size);
        out_frame->pts=i;
        result=encode(out_codecs_cxt,out_frame,&out_packet,&pcmWriter);
        if (result<0){
            av_log(NULL,AV_LOG_ERROR,"编码异常:%s", av_err2str(result));
            return -1;
        }
        ///////////////编码的代码结束

//        av_log(NULL,AV_LOG_INFO,"read size=%d,",packet.size);

        av_packet_unref(&in_packet);

    }
    encode(out_codecs_cxt,NULL,&out_packet,&pcmWriter);
//    waveWriter.WriteTail();

//    av_freep(dest[0]);
    av_freep(raw_data);
//    av_freep(src[0]);
    av_freep(in_data);


    swr_free(&swrContext);
    avformat_free_context(cxt);
    //编码器的
    av_frame_free(&out_frame);
    avcodec_free_context(&out_codecs_cxt);
}

int encode(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket, YUVWriter * writer) {
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
            writer->WritePackage(pPacket);
            printPackageInfo(pPacket,iters);
            av_packet_unref(pPacket);
    }
}
