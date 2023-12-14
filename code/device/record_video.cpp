

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#ifdef __cplusplus
}
#endif



//这里内默认输入是2940*1912，nv12
// WritePackage 转换成yuv420后输出到文件
class YUVWriter{
public:
    YUVWriter(const char *prefix){
        char filename[256];
        sprintf(filename,"%s.yuv",prefix);
        fp= fopen(filename,"wb");
    }
    bool WritePackage(AVPacket* packet){
//        int size=2940*1912+2940*1912/2; //
//        return fwrite(packet->data,size,1,fp)>0;

        //nv12  YYYY YYYY YYYY YYYY UVUV UVUV
        //420p  YYYY YYYY YYYY YYYY UUUU VVVV

        int ysize=2940*1912;
        int usize=2940*1912/4;
        fwrite(packet->data,ysize,1,fp);

        uint8_t* uv_420p_data=new uint8_t [usize+usize];
        uint8_t* uv_nv12_data=packet->data+ysize;

        for (int i = 0; i < usize; ++i) {
            uv_420p_data[i]=uv_nv12_data[2*i];
            uv_420p_data[i+usize]=uv_nv12_data[2*i+1];
        }
        fwrite(uv_420p_data,usize*2,1,fp);
        return 1;
    }
    virtual ~YUVWriter(){
        fclose(fp);
    }
private:
    FILE *fp;
};

//本程序演示了如何把camera上的的图像 保存成video.pcm与video.wav
//播放
// ffplay -video_size 640x480 -pixel_format nv12 video.yuv

#define SAMPLES 600
#define FILE_PREFIX "video"
#define DEVICE_NAME "1"

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
    //0，摄像头，1桌面
    const char *deviceName=DEVICE_NAME;

    AVDictionary* option=NULL;
    av_dict_set(&option,"framerate","30",0);
    av_dict_set(&option,"video_size","640x480",0);
    //supported pixel formats For mac:
    //uyvy422,yuyv422,nv12,rgb,rgb,bgr0,rgb
    av_dict_set(&option,"pixel_format","nv12",0);

    int result=avformat_open_input(&cxt,deviceName,inputFormat,&option);
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


    av_log(NULL,AV_LOG_WARNING,"stream[0]\n编码器 %s,%s,\n分辨率 %dx%d,fps %d,像素格式 %s,比特率 %ld\n",
           codecs->name,
           codecs->long_name,
           codec_context->width,
           codec_context->height,
           codec_context->framerate,
           av_get_pix_fmt_name(codec_context->pix_fmt),
           codec_context->bit_rate
    );
//    av_get_pix_fmt_name(codec_context->pix_fmt),
    av_log(NULL,AV_LOG_WARNING,"ffplay -video_size %dx%d -pixel_format yuv420p %s.yuv\n",
           codec_context->width,
           codec_context->height,
           FILE_PREFIX
    );
//
    ////////////////////////////////////////////////////////////////////////



    ////////////////////////////读取设备数据////////////////////////////
    AVPacket packet;
    av_init_packet(&packet);//除了data,size的字段的初始化

    //4.读取设备包
    YUVWriter pcmWriter(FILE_PREFIX);

    //录制300个包
    for (int i = 0; i < SAMPLES; ++i) {
        if (av_read_frame(cxt,&packet)<0){
            av_log(NULL,AV_LOG_ERROR,"读取包失败");
            break;
        }

        pcmWriter.WritePackage(&packet);

//        av_log(NULL,AV_LOG_INFO,"read size=%d,",packet.size);
        av_packet_unref(&packet);
//        av_log(NULL,AV_LOG_INFO,"after unref size=%d\n",packet.size);
    }



    avformat_free_context(cxt);

}