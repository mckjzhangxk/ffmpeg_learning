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

//本程序向你展示了一个如何把h264再次封装的过程。
//由于h264是裸数据，所以读出来的packet pts,流的time_base
//我们通过2中方法，来重新设置 pts,time_base

//遗留的问题：如何从h264 sps获得fps?
int main(int argc, char **argv)
{
    int ret;

    if (argc < 3) {
        printf("usage remuxing-h264 a.h264 a.ts\n", argv[0]);
        return 1;
    }

    const char * in_filename  = argv[1];
    const char * out_filename = argv[2];

    av_register_all();

    AVFormatContext *ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, av_find_input_format("h264"), 0)) < 0) {
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



    //流的基本信息
    int stream_index = av_find_best_stream(ifmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,NULL);
    AVStream *in_stream = ifmt_ctx->streams[stream_index];
    AVStream * out_stream = avformat_new_stream(ofmt_ctx, NULL);


    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream[stream_index].codecpar);
    if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters\n");
        return -1;
    }
    out_stream->codecpar->codec_tag = 0;




    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    //当输出格式的 AVFMT_NOFILE标志=1,调用者不需要提供一个AVIOContext,比如m3u8
    //否则 AVFMT_NOFILE标志=0，调用者需要用avio_open 提供一个AVIOContext,比如mp4

    int no_need_provide_io_context =ofmt_ctx->oformat->flags & AVFMT_NOFILE;

    if (!no_need_provide_io_context) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            return -1;
        }
    }


    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }

    //拷贝包的操作
    long total_duration=0;
    int steps=0;
    for(int k=0;k<100;k++){
        //需要时间基
        AVPacket pkt;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0){
            av_log(NULL,AV_LOG_ERROR,"%s",av_err2str(ret));
            break;
        }
        if (stream_index!=pkt.stream_index) {
            av_packet_unref(&pkt);
            continue;
        }



        AVStream  *out_stream = ofmt_ctx->streams[0];
        pkt.stream_index = 0;

        //方法1 由于duration设置的没有问题，我们可以把duration的累计作为pts
//        AVStream * in_stream  = ifmt_ctx->streams[stream_index];
//        pkt.pts=total_duration;
//        log_packet(ifmt_ctx, &pkt, "in");
//        int src_duration=pkt.duration;
//        av_packet_rescale_ts(&pkt,in_stream->time_base, out_stream->time_base);
//        total_duration+=src_duration;
        //方法1结束//////////////////////////////

        //方法2 手动设置pts,然后自定义fps
        pkt.pts=steps++;
        pkt.dts= pkt.pts;
        pkt.duration=0;
        AVRational time_base={1,5};
        av_packet_rescale_ts(&pkt,time_base, out_stream->time_base);
        log_packet(ofmt_ctx, &pkt, "out");
        //方法2结束



        pkt.pos = -1;
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
