#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif


#include "libavutil/log.h"
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"

#ifdef __cplusplus
}
#endif
//查看本文章
///https://wanggao1990.blog.csdn.net/article/details/114067251?spm=1001.2014.3001.5502

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

//生成输出上下文的操作
// avformat_alloc_output_context2
// avformat_free_context

//stream
//stream=streamId + codes parameter +time_base
//创建流函数 与拷贝参数函数
//avformat_new_stream
//avcodec_parameters_copy


//  把package写出去的操作
//    avformat_write_header();
//    av_interleaved_write_frame()
//    av_write_frame()
//    av_write_trailer()

//一个有趣的例子是
//A) 输入yogo.mp4(60fps)==>yogo.h264(60fps)
//B) 输入tvt.mp4(4.67 fps)==>tvt.h264(25fps)
//出现以上现象的原因是因为tvt.mp4 里面的h264没有正确设置sps的frame，导致输出的tvt.h264 time_scale,time_tick_num都是0
//所以ffplay只能使用默认的25fps播放。

//另外一个有趣的现象是.h264不能转换成其他任何封装
//原因1）：h264的stream 的codeParameter没有time_base，拷贝给目标ctx也就没有，目标自然会在write的时候出错，
//原因2）:h264的sps,pts,dts设置有误，如tvt.h264
int main(int argc, char **argv)
{
    int ret, i;

    if (argc < 3) {
        printf("usage: %s input output\n"
               "API example program to remux a media file with libavformat and libavcodec.\n"
               "The output format is guessed according to the file extension.\n"
               "\n", argv[0]);
        return 1;
    }

    const char * in_filename  = argv[1];
    const char * out_filename = argv[2];

    av_register_all();

    AVFormatContext *ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return -1;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    AVFormatContext  *ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);


    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return -1;
    }



    //输入与输出之间的对应关系
    int  stream_mapping[4]={-1,-1,-1,-1};


    //流的基本信息
    int stream_index = 0;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {

        AVStream *in_stream = ifmt_ctx->streams[i];

        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (!avformat_query_codec(ofmt_ctx->oformat,in_codecpar->codec_id,FF_COMPLIANCE_NORMAL)){
            continue;
        }

        stream_mapping[i] = stream_index++;

        AVStream * out_stream = avformat_new_stream(ofmt_ctx, NULL);

        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            return -1;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    //当输出格式的 AVFMT_NOFILE标志=1,调用者不需要提供一个AVIOContext,比如m3u8
    //否则 AVFMT_NOFILE标志=0，调用者需要用avio_open 提供一个AVIOContext,比如mp4

    int no_need_provide_io_context =ofmt_ctx->oformat->flags & AVFMT_NOFILE;

    if (!no_need_provide_io_context) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }


    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
//    av_packet_from_data
    //拷贝包的操作
    while (1) {
        //需要时间基
        AVPacket pkt;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0){
            av_log(NULL,AV_LOG_ERROR,"%s",av_err2str(ret));
            break;
        }


        AVStream * in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        AVStream  *out_stream = ofmt_ctx->streams[pkt.stream_index];
        log_packet(ifmt_ctx, &pkt, "in");

        /* copy packet */

        av_packet_rescale_ts(&pkt,in_stream->time_base, out_stream->time_base);
//        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
//        log_packet(ofmt_ctx, &pkt, "out");

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);
end:

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);



    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
