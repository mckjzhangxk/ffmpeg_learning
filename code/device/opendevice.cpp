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

//本代码想展示如何打开一个音频设备,读取设备流，存储到文件
//流程如下：
// .
// 1.注册 所有设备
// 2.设置采集方式avfordation(mac),alsa(linux),dshow(windows)

//  可以通过以下命令查看设备驱动
//  ffmpeg -formats|grep device
//       D  avfoundation    AVFoundation input device
// 3.打开设备


//4.读取设备包

// A.值得注意的，我们对包的初始化与是否分成在stack与在heap,两种内存模型有各自对应的函数
// 栈操作
//    av_init_packet， //初始化除了 出data,size外的数据为默认值
//    av_packet_unref //计数器减1,我的理解是释放掉data的内存

// 堆操作
//    av_packet_alloc() //分配空间，并初始化
//    av_packet_free() //计数器减1,释放空间

/*   //堆上的操作
 *   while(1){
 *      AVPackage *pt=av_packet_alloc();
 *      av_read_frame(ctx,pt)
 *      av_packet_free(pt);
 *   }
 * */

/*   //栈上的操作
 *   AVPackage pt;
 *   av_init_packet(&pt);
 *   while(1){
 *      av_read_frame(ctx,pt)
 *      av_packet_unref(&pt);
 *   }
 * */

//B.av_packet_unref函数的具体作用
// av_read_package应该会在heap 生成data空间，我们调用av_packet_unref,是
// 为了把这个空间释放掉


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
    cxt->audio_codec->name;
    ////////////////////////////////////////////////////////////////////////



    ////////////////////////////读取设备数据////////////////////////////
    AVPacket packet;
    av_init_packet(&packet);//除了data,size的字段的初始化

    //4.读取设备包
    for (int i = 0; i < 3; ++i) {
        if (av_read_frame(cxt,&packet)<0){
            av_log(NULL,AV_LOG_ERROR,"读取包失败");
            break;
        }

        av_log(NULL,AV_LOG_INFO,"read size=%d,",packet.size);
        av_packet_unref(&packet);
        av_log(NULL,AV_LOG_INFO,"after unref size=%d\n",packet.size);
    }
//    av_packet_unref(&packet);


    avformat_free_context(cxt);

}