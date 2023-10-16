#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter//avfilter.h"

#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"

#ifdef __cplusplus
}
#endif




/**
 * @brief open media file
 * @param filename xxx
 * @param [out] fmt_ctx xxx
 * @param [out] dec_ctx xxx
 * @return 0: success, <0: failure
 */
static int open_input_file(const char *filename,
                           AVFormatContext*  &fmt_ctx,
                           AVCodecContext*   &dec_ctx){

    int ret = -1;
    ////////////输入上下文////////////////////////////
    if(( ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to open file %s\n", filename);
        return ret;
    }

    if((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information!\n");
        return ret;
    }

    ////////////解码器上下文////////////////////////////
    //之前是通过  avcodec_find_decoder函数获得的codec,这里演示了使用find_best_stream可以
    //从输入提取解码器
    AVCodec *codec = NULL;
    if ((ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0){
        av_log(NULL, AV_LOG_ERROR, "Can't find video stream!\n");
        return ret;
    }
    dec_ctx = avcodec_alloc_context3(codec);
    if(!dec_ctx){
        return AVERROR(ENOMEM);
    }
    //对于流，常用的操作是avcodec_parameters_copy()
    //对于codec context，使用avcodec_parameters_to_context,拷贝的是W,H,FPS，format等基础信息
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[0]->codecpar);

    if((ret = avcodec_open2(dec_ctx, codec, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to open decoder!\n");
        return ret;
    }

    return 0;
}


/**
 * @brief
 * @param filter_desc filter的描述符
 * @param [in] fmt_ctx 输入上下文
 * @param [in] dec_ctx 解码器
 * @param [out] graph  构造的AVFilterGraph
 * @param [out] bufsource_ctx  filter是输入上下文
 * @param [out] bufsink_ctx filter是输出上下文
 * @return 0: success, <0: failure
 */
static int init_filters(const char *filter_desc,
                        AVFormatContext *fmt_ctx,
                        AVCodecContext *decoder,
                        AVFilterGraph*  &graph,
                        AVFilterContext*& bufsource_ctx,
                        AVFilterContext* &bufsink_ctx){
    //////////创建图
    graph = avfilter_graph_alloc();

    /////////初始化buffer source filter
    const AVFilter* bufsource= avfilter_get_by_name("buffer");

    char args[512] = {}; //初始化buffer filter的参数
    // ffmpeg -h filter=buffer 可以查看需要设置的参数
    snprintf(args,512,"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             decoder->width,decoder->height,
             decoder->pix_fmt,
             fmt_ctx->streams[0]->time_base.num,fmt_ctx->streams[0]->time_base.den,
             decoder->sample_aspect_ratio.num,decoder->sample_aspect_ratio.den
             );
    //创建filter context
    if (avfilter_graph_create_filter(&bufsource_ctx,bufsource,"in",args,NULL,graph)<0){
        av_log(NULL, AV_LOG_ERROR, "无法创建buffer source!\n");
        return -1;
    }

    


    /////////初始化buffer sink filter
    const AVFilter *bufsink = avfilter_get_by_name("buffersink");
    //ffmpeg -h filter=buffersink, option需要binary ,pix_fmts:<binary>

    if (avfilter_graph_create_filter(&bufsink_ctx,bufsink,"out",NULL,NULL,graph)<0){
        av_log(NULL, AV_LOG_ERROR, "无法创建buffer sink!\n");
        return -1;
    }
    //设置输出支持的格式。AV_PIX_FMT_NONE是结束元素
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};
    av_opt_set_int_list(bufsink_ctx,"pix_fmts",pix_fmts,AV_PIX_FMT_NONE,AV_OPT_SEARCH_CHILDREN);

    /////////创建输入，输出
    AVFilterInOut *inputs = avfilter_inout_alloc();

    //不理解这里的配置，感觉反了？
    //输入 buffer filter
    //"[in]drawbox=xxxx[out]"
    inputs->name = av_strdup("out"); //复制字符串
    inputs->filter_ctx = bufsink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    AVFilterInOut *outputs = avfilter_inout_alloc();
    outputs->name = av_strdup("in");
    outputs->filter_ctx = bufsource_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    /////////分析filter描述符,并构建AVFilterGrapg

    if(avfilter_graph_parse(graph,filter_desc,inputs,outputs,NULL)<0){
        av_log(NULL, AV_LOG_ERROR, "无法创建graph!\n");
        return -1;
    }

    //////////让构建好的 graph生效
    if(avfilter_graph_config(graph,NULL)<0){
        av_log(NULL, AV_LOG_ERROR, "无法让grap生效!\n");
        return -1;
    }

//    会报错，为什么？
//    avfilter_inout_free(&inputs);
//    avfilter_inout_free(&outputs);
    return 0;
}

//解码视频帧并对视频帧进行滤镜处理
static int decode_frame_and_filter(
                                   AVCodecContext *decoder,
                                   AVFilterContext *bufsource_ctx,
                                   AVFilterContext *bufsink_ctx,
                                   AVFrame *frame,
                                   AVFrame *frame_flt,
                                   FILE* out){


    int ret = avcodec_receive_frame(decoder, frame);
    if(ret < 0){
        return ret;
    }
    //滤镜处理，先发送给source buffer
    if((ret=av_buffersrc_add_frame(bufsource_ctx,frame)<0)){
        return ret;
    }

    while (1){
        int ret=av_buffersink_get_frame(bufsink_ctx,frame_flt);

        if (ret<0){
            break;
        }
        //只保存灰度
        fwrite(frame_flt->data[0],1,frame_flt->width*frame_flt->height,out);
//        fwrite(frame_flt->data[1],1,frame_flt->width*frame_flt->height/4,out);
//        fwrite(frame_flt->data[2],1,frame_flt->width*frame_flt->height/4,out);
        av_frame_unref(frame_flt);
    }

    av_frame_unref(frame);
    return ret;

}
/**
 * 关于filter api 的数据结构
 *
 * AVFilterGraph: 整个filter的 DAG
 * AVFilter: 具体的某个filter,比如Overlay.DrawBox
 * AVFilterContext: 关联AVFilter，用于具体的滤镜操作。
 *
 * AVFilter与AVFilterContext的关系 类似于 codec与codec context的关系。
 *
 * bufferFilter: 特殊的AVFilter,表示source
 * bufferSinkFilter: 特殊的AVFilter,表示sink
 *
 * */
 //ffplay -video_size 1280x720  -pixel_format yuv420p jojo_s2.yuv
 //ffplay -video_size 1280x720  -pixel_format gray8 jojo_s2.yuv
int main(int argc, const char * argv[]){
    av_log_set_level(AV_LOG_DEBUG);

    const char* filename = "/Users/zhanggxk/Desktop/测试视频/jojo_s2.mp4";
    const char* filename_out = "/Users/zhanggxk/Desktop/测试视频/jojo_s2.yuv";

    AVFormatContext *fmt_ctx=NULL;
    AVCodecContext *decoder=NULL;
    if(open_input_file(filename,fmt_ctx,decoder)<0){
        return -1;
    }


    //filter描述符,x=30,y=10,w=64,h=64,color=red
    const char *filter_desc="drawbox=30:10:64:64:red";

    AVFilterGraph* graph=NULL;
    AVFilterContext * buffersource=NULL,*buffersink=NULL;

    if(init_filters(filter_desc,fmt_ctx,decoder,graph,buffersource,buffersink)<0){
        return -1;
    }

    AVPacket packet;
    AVFrame *frame=av_frame_alloc();
    AVFrame *frame_flt=av_frame_alloc();
    FILE * out= fopen(filename_out,"wb");

    while(1){
        if((av_read_frame(fmt_ctx, &packet)) < 0){
            break;
        }
        if(packet.stream_index != 0)
            continue;

        int ret = avcodec_send_packet(decoder, &packet);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "解码发送失败!\n");
            break;
        }

        ret=decode_frame_and_filter(decoder,buffersource,buffersink,frame,frame_flt,out);

        if (ret<0){
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                continue;
            }
            av_log(NULL, AV_LOG_ERROR, "解码接收失败!\n");
            break;
        }

    }



    ///////////////////////////清理逻辑///////////////////////////
    fclose(out);

    if(decoder){
        avcodec_free_context(&decoder);
    }

    if(fmt_ctx){
        avformat_close_input(&fmt_ctx);
    }
    if(frame){
        av_frame_free(&frame);
    }
    if(frame_flt){
        av_frame_free(&frame_flt);
    }

}