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
#include <string>
using std::string;
void  print_codec_info(const char *title, AVCodecContext * in_codes_ctx){
    AVCodec * codecs=avcodec_find_encoder(in_codes_ctx->codec_id);

    av_log(NULL, AV_LOG_WARNING, "\t%s\n编码器 %s,%s,\n声道数 %d,sample rate %d,位深 %dbits,比特率 %ld\n",
           title,
           codecs->name,
           codecs->long_name,
           in_codes_ctx->channels,
           in_codes_ctx->sample_rate,
           av_get_bytes_per_sample(in_codes_ctx->sample_fmt),
           in_codes_ctx->bit_rate
    );
}

class Writer{
public:
    Writer(const char *filename){
        fp= fopen(filename,"wb");
    }
    bool Write(uint8_t* data,int size){
        return fwrite(data,size,1,fp)>0;
    }
    virtual ~Writer(){
        fclose(fp);
    }
private:
    FILE *fp;
};

class Reader{
public:
    Reader(const char *filename){
        fp= fopen(filename,"rb");
    }
    int Read(uint8_t* data,int size){
        return fread(data,1,size,fp);
    }
    virtual ~Reader(){
        fclose(fp);
    }
private:
    FILE *fp;
};


//声道布局的名字
string get_layout_name(uint64_t layout){
    char buf[1024];
    av_get_channel_layout_string(buf,1024,0,layout);

    string layout_name=buf;
    return layout_name;
}
//声道布局的声道数量
int get_layout_channels(uint64_t layout) {
    return av_get_channel_layout_nb_channels(layout);
}


// 虽然代码很简单，但是由于以下问题，让我花了一个下午的调试时间。

//bug1 :指针问题，以下代码
//int n_per_channel=swr_convert(swrContext,
//                              out_data,out_samples,
//                              (const uint8_t **)in_data,in_samples); //写成了 (const uint8_t **)&in_data，导致输出混乱

//bug2  fltp与flt的存储是有区别的，fltp后，av_samples_alloc_array_and_samples 中channel的变化不会影响输出的linesize

//bug3 swr_convert 接受的输入是 uint8 **,原因是不论是对于图片还是声音，数据的格式如下
// ch1-->uint8 *[.....]
// ch2-->uint8 *[.....]
// ch3-->uint8 *[.....]

//bug4 为什么我把段地址的地主传给 swr_convert会报错
//    uint8_t *PACKET_BUFFER=new uint8_t[PACK_SIZE]; //正常
//    uint8_t PACKET_BUFFER[PACK_SIZE];   // 段异常

//int n_per_channel=swr_convert(swrContext,
//                              out_data,out_samples,
//                              (const uint8_t **)PACKET_BUFFER,in_samples);

#define PACK_SIZE 2048*2


//以下是输出PCM 重采样的音频参数
#define AUDIO_CHANNEL_LAYOUT     AV_CH_LAYOUT_STEREO
#define AUDIO_SAMPLE_RATE        44100
#define ADUIO_FORMAT             "s16"

//以下是最简单的重采样函数。
//把 48000,1，float32的输入文件voice.pcm 转换成上面参数设置的音频，输出
int main(int argc,char* argv[]){
    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();

    string infile="/Users/zhanggxk/Desktop/测试视频/jojo-48k-1-flt.pcm";
    string outfile="voice-";
    outfile+=std::to_string(AUDIO_SAMPLE_RATE)+"-";
    outfile+= get_layout_name(AUDIO_CHANNEL_LAYOUT)+"-";
    outfile+=ADUIO_FORMAT;
    outfile+=".pcm";



    ////////////////////////////////////////输出文件的信息////////////////////////////////////////
    AVSampleFormat sample_fmt= av_get_sample_fmt(ADUIO_FORMAT);
    int sample_size= av_get_bytes_per_sample(sample_fmt);
    int channels= get_layout_channels(AUDIO_CHANNEL_LAYOUT);


    av_log(NULL,AV_LOG_INFO,"输出采样率 %d,声道数 %d,采样格式 %s,采样大小 %d，输出文件 %s\n",
           AUDIO_SAMPLE_RATE,
           channels,
           ADUIO_FORMAT,
           sample_size,
           outfile.c_str()
           );
    av_log(NULL,AV_LOG_INFO,"ffplay -ar %d -ac %d -f %s  %s\n",
           AUDIO_SAMPLE_RATE,
           channels,
           ADUIO_FORMAT,
           outfile.c_str()
    );


    Reader reader(infile.c_str());
    Writer writer(outfile.c_str());


    //////////////重采样上下文设置，输入输出 音频三要素设置///////////////////

//    SwrContext *swrContext= swr_alloc_set_opts(NULL,
//                                               AUDIO_CHANNEL_LAYOUT, sample_fmt, AUDIO_SAMPLE_RATE,
//                                               AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, 48000,
//                                               0, NULL);
    SwrContext *swrContext = swr_alloc();
    av_opt_set_int(swrContext, "in_channel_layout", AV_CH_LAYOUT_MONO, 0);
    av_opt_set_int(swrContext, "in_sample_rate", 48000, 0);
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    av_opt_set_int(swrContext, "out_channel_layout", AUDIO_CHANNEL_LAYOUT, 0); // 目标通道布局（立体声）
    av_opt_set_int(swrContext, "out_sample_rate", AUDIO_SAMPLE_RATE, 0); // 目标采样率
    av_opt_set_sample_fmt(swrContext, "out_sample_fmt", sample_fmt, 0); // 目标采样格式（16位有符号整数）

    if(!swrContext){
        av_log(swrContext,AV_LOG_ERROR,"无法创建重采样上下文");
        return -1;
    }

    if (swr_init(swrContext)<0){
        av_log(swrContext,AV_LOG_ERROR,"无法初始化重采样上下文");
        return -1;
    }

    ///////////分配输出的缓冲区大小///////////////////
    //这里是计算 输入的样本数，输入 48000hz fmt:f32le channel:1
    int in_samples=PACK_SIZE/1/4;
    uint8_t *in_data=new uint8_t[PACK_SIZE];//uint8_t in_data[PACK_SIZE]; //出错，为什么？

    int out_samples=in_samples*AUDIO_SAMPLE_RATE/48000;
    //如果觉得下面ffmpeg的分配代码过于笨拙，可以等效成以下代码
    int out_line_size=out_samples*sample_size*channels;
    uint8_t* out_data=new uint8_t[out_line_size];
//    uint8_t** out_data=NULL;
//    int out_line_size;
//    av_samples_alloc_array_and_samples(&out_data,&out_line_size,channels,out_samples,sample_fmt,0);//align=1表示不对齐，0表示自动对齐




    ///////////原始数据的重采用///////////////////
    int n;
    while ((n=reader.Read(in_data, PACK_SIZE)) > 0){
        int n_per_channel=swr_convert(swrContext,
                                      &out_data, out_samples,
                                      (const uint8_t **)&in_data, in_samples);

        if (n_per_channel<0){
            av_log(NULL,AV_LOG_ERROR,"转换失败");
            return -1;
        }
        writer.Write(out_data,out_line_size);
    }

    delete out_data;
    delete in_data;
    swr_free(&swrContext);
}