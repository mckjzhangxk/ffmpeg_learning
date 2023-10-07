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
#include "libswscale/swscale.h"
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







//以下是输出PCM 重采样的音频参数
#define OUT_WIDTH     800
#define OUT_HEIGHT    520

#define IN_WIDTH      2940
#define IN_HEIGHT     1912

//本程序使用device_record_video 生成video.yuv.
//video.yuv是yuv是紧密排列在一起的

//然后调用sws_getContext创建上下文
//sws_scale，进行转换、
//重点关注数据的分配与格式的组织。
int main(int argc,char* argv[]){
    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();

    string infile="video.yuv";
    string outfile="video-";
    outfile+=std::to_string(OUT_WIDTH)+"-";
    outfile+= std::to_string(OUT_HEIGHT);
    outfile+=".yuv";



    ////////////////////////////////////////输出文件的信息////////////////////////////////////////
    av_log(NULL,AV_LOG_INFO,"ffplay -video_size %dx%d   -pixel_format yuv420p %s\n",
           OUT_WIDTH,
           OUT_HEIGHT,
           outfile.c_str()
    );


    Reader reader(infile.c_str());
    Writer writer(outfile.c_str());


    //////////////重采样上下文设置，输入输出 音频三要素设置///////////////////


    SwsContext *swrContext=sws_getContext(IN_WIDTH,IN_HEIGHT,AV_PIX_FMT_YUV420P,
                                          OUT_WIDTH,OUT_HEIGHT,AV_PIX_FMT_YUV420P,
                                          SWS_BILINEAR,NULL,NULL,NULL);

    if(!swrContext){
        av_log(swrContext,AV_LOG_ERROR,"无法创建重采样上下文");
        return -1;
    }



    ///////////分配输出的缓冲区大小///////////////////
    //一个frame 3个通道，linesize表示一行扫描线下通道的样本点，y->width,u->width/2,v->width/2
    int in_linesize[3]={IN_WIDTH,IN_WIDTH/2,IN_WIDTH/2};
    int out_linesize[3]={OUT_WIDTH,OUT_WIDTH/2,OUT_WIDTH/2};

    uint8_t * in_frame_data[3]={new uint8_t [IN_WIDTH*IN_HEIGHT],new uint8_t [IN_WIDTH*IN_HEIGHT/4],new uint8_t [IN_WIDTH*IN_HEIGHT/4]};
    uint8_t * out_frame_data[3]={new uint8_t [OUT_WIDTH*OUT_HEIGHT],new uint8_t [OUT_WIDTH*OUT_HEIGHT/4],new uint8_t [OUT_WIDTH*OUT_HEIGHT/4]};




    ///////////原始数据的重采用///////////////////
    int n;
    while ((n=reader.Read(in_frame_data[0], IN_WIDTH*IN_HEIGHT)) > 0){
        n=reader.Read(in_frame_data[1], IN_WIDTH*IN_HEIGHT/4);
        n=reader.Read(in_frame_data[2], IN_WIDTH*IN_HEIGHT/4);

        int h=sws_scale(swrContext,in_frame_data,in_linesize,0,IN_HEIGHT,
                  out_frame_data,out_linesize);

        writer.Write(out_frame_data[0],OUT_WIDTH*OUT_HEIGHT);
        writer.Write(out_frame_data[1],OUT_WIDTH*OUT_HEIGHT/4);
        writer.Write(out_frame_data[2],OUT_WIDTH*OUT_HEIGHT/4);
    }

    delete in_frame_data[0];
    delete in_frame_data[1];
    delete in_frame_data[2];

    delete out_frame_data[0];
    delete out_frame_data[1];
    delete out_frame_data[2];
    sws_freeContext(swrContext);
}