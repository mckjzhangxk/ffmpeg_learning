#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"

#ifdef __cplusplus
}
#endif



class PCMWriter{
public:
    PCMWriter(const char *prefix){
        char filename[256];
        sprintf(filename,"%s.pcm",prefix);
        fp= fopen(filename,"wb");
    }
    bool WritePackage(AVPacket* packet){
        return fwrite(packet->data,packet->size,1,fp)>0;
    }
    virtual ~PCMWriter(){
        fclose(fp);
    }
private:
    FILE *fp;
};
class WaveWriter{
public:
    WaveWriter(const char *prefix,AVCodecParameters *code_params):cxt(NULL){
        char filename[256];
        sprintf(filename,"%s.wav",prefix);

        //分配输出上下文
        avformat_alloc_output_context2(&cxt,NULL,NULL,filename);
        //需要提供输出IO
        if (!(cxt->oformat->flags&AVFMT_NOFILE)){
            avio_open(&cxt->pb,filename,AVIO_FLAG_WRITE);
        }
        //创建新的流，拷贝codec参数
        //注意如果这里手动设置会很麻烦
        AVStream* stream=avformat_new_stream(cxt,NULL);
        int ret=avcodec_parameters_copy(stream->codecpar,code_params);
        if (ret<0){
            av_log(NULL,AV_LOG_ERROR,"拷贝编码器参数异常");
            return;
        }
        av_dump_format(cxt,1,filename,1);

    }
    bool WritePackage(AVPacket* packet){
        return  av_write_frame(cxt,packet)==0;
    }

    void WriteHeader(){
        int ret=avformat_write_header(cxt,NULL);
        if (ret<0){
            av_log(NULL,AV_LOG_ERROR,"写头部异常");
        }
    }
    void WriteTail(){
        av_write_trailer(cxt);
    }
    virtual ~WaveWriter(){
        avio_closep(&cxt->pb);
        avformat_free_context(cxt);
    }
private:
    AVFormatContext* cxt;
};
//本程序演示了如何把mic上的的声音 保存成voice.pcm与voice.wav
//播放 pcm
// ffplay -ar 48000 -f f32le -ac 1 voice.pcm

//重采样
//ffmpeg -ar 48000 -f f32le -ac 1 -i  voice.pcm  -ar 48000 -f f32le -ac 2  voice-new.pcm
// ffplay -ar 48000 -f f32le -ac 2 voice-new.pcm

//
//ffmpeg -ar 48000 -f f32le -ac 1 -i  voice.pcm  -ar 44100 -f s16le -ac 2  voice-new.pcm
// ffplay -ar 44100 -f s16le -ac 2 voice-new.pcm
#define SAMPLES 500
int main(){
    char errbuf[128]={0};
    av_log_set_level(AV_LOG_DEBUG);
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

    AVCodecContext * codec_context=cxt->streams[0]->codec;
    AVCodec * codecs=avcodec_find_encoder(codec_context->codec_id);

    av_log(NULL,AV_LOG_WARNING,"stream[0]\n编码器 %s,%s,\n声道数 %d,sample rate %d,位深 %dbits,比特率 %ld\n",
           codecs->name,
           codecs->long_name,
           codec_context->channels,
           codec_context->sample_rate,
           codec_context->bits_per_coded_sample,
           codec_context->bit_rate
    );

    ////////////////////////////////////////////////////////////////////////



    ////////////////////////////读取设备数据////////////////////////////
    AVPacket packet;
    av_init_packet(&packet);//除了data,size的字段的初始化

    //4.读取设备包
    PCMWriter pcmWriter("voice");
    WaveWriter waveWriter("voice",cxt->streams[0]->codecpar);

    waveWriter.WriteHeader();
    //录制300个包
    for (int i = 0; i < SAMPLES; ++i) {
        if (av_read_frame(cxt,&packet)<0){
            av_log(NULL,AV_LOG_ERROR,"读取包失败");
            break;
        }

        pcmWriter.WritePackage(&packet);
        waveWriter.WritePackage(&packet);
//        av_log(NULL,AV_LOG_INFO,"read size=%d,",packet.size);
        av_packet_unref(&packet);
//        av_log(NULL,AV_LOG_INFO,"after unref size=%d\n",packet.size);
    }

    waveWriter.WriteTail();


    avformat_free_context(cxt);

}