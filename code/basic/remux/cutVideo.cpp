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

int main(int argc, char **argv)
{
    int ret, i;

    if (argc < 3) {
        printf("usage:cutVideo infile outfile start end\n"
               "\n", argv[0]);
        return 1;
    }

    const char * in_filename  = argv[1];
    const char * out_filename = argv[2];

    double starttime= atoi(argv[3]);
    double endtime= atoi(argv[4]);
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

    if(av_seek_frame(ifmt_ctx,-1,starttime*AV_TIME_BASE,AVSEEK_FLAG_ANY)){
        fprintf(stderr, "无法正常的seek\n");
        goto end;
    }

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


        double t=pkt.pts*av_q2d(in_stream->time_base);
        if (t>endtime){
            break;
        }
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
