#ifdef __cplusplus
    #ifndef __STDC_CONSTANT_MACROS
    #define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif
    #include "libavutil/log.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libavutil/timestamp.h"
#ifdef __cplusplus
}
#endif

#include <string>
#include <vector>



int encode(AVCodecContext* ctx,AVPacket *packet,AVFrame *frame,AVFormatContext* out_ctx){
    static int steps=0;

    steps++;
    int result=avcodec_send_frame(ctx,frame);
    if (result<0){
        av_log(NULL,AV_LOG_ERROR,"encoder error:%s",av_err2str(result));
        return -1;
    }

    //while循环是因为编码器 可能会吐出多个包


    while (result>=0){
        av_init_packet(packet);
        result= avcodec_receive_packet(ctx,packet);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            av_packet_unref(packet);
            return 0;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
            av_packet_unref(packet);
            return -1;
        }

        av_write_frame(out_ctx,packet);
        av_packet_unref(packet);
    }

}

int decode(AVCodecContext* ctx,AVFrame *frame,AVPacket *packet){


    int result=avcodec_send_packet(ctx,packet);
    if (result<0){
        av_log(NULL,AV_LOG_ERROR,"video_decode error:%s",av_err2str(result));
        return -1;
    }

    //while循环是因为编码器 可能会吐出多个包


    if (result>=0){
        result=avcodec_receive_frame(ctx,frame);
        if (result== AVERROR(EAGAIN) ||result== AVERROR_EOF){
            //编码器没有足够的数据进行输出到package
            return 0;
        }else if(result<0){
            // codec not opened, or it is an encoder other errors: legitimate decoding errors
            //编码器其他异常
            av_log(NULL,AV_LOG_ERROR,"%s", av_err2str(result));
            return -1;
        }
        return 1;
    }

}


//很程序查看 packet的数据包的pts,dts,duratio，并且与解码后的数据帧 做躲避
int transcode(const char * infile,const char * outfile){


    AVFormatContext *out_ctx=NULL;
    avformat_alloc_output_context2(&out_ctx, NULL, NULL, outfile);
    int no_need_provide_io_context =out_ctx->oformat->flags & AVFMT_NOFILE;

    if (!no_need_provide_io_context) {
        int ret = avio_open(&out_ctx->pb, outfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_ctx);
            return -1;
        }
    }


    if (!out_ctx){
        return -1;
    }

    AVFormatContext *ctx=NULL;

    av_register_all();
    av_log_set_level(AV_LOG_DEBUG);
    if(avformat_open_input(&ctx,infile,NULL,NULL)<0){
        return -1;
    }

    if(avformat_find_stream_info(ctx,NULL)<0){
        return -1;
    }


    AVRational* time_base=&ctx->streams[0]->time_base;
    av_log(NULL,AV_LOG_INFO,"流的时间基 %d/%d\n",time_base->num,time_base->den);


    AVFrame * frame=av_frame_alloc();


    //////////////////////////////////////////////////////////////////////
    AVCodec * decoder=avcodec_find_decoder(ctx->streams[0]->codecpar->codec_id);

    if (!decoder){
        return -1;
    }

    AVCodecContext* decoder_ctx= avcodec_alloc_context3(decoder);
    if (!decoder_ctx){
        return -1;
    }

    avcodec_parameters_to_context(decoder_ctx,ctx->streams[0]->codecpar);


    if(avcodec_open2(decoder_ctx,decoder,NULL)){
        return -1;
    }
    /////////////////////////////////////////////////////////////////////////

    AVCodec *encoder= avcodec_find_encoder_by_name("libx264");
    if (!encoder){
        return -1;
    }
    AVCodecContext* encoder_ctx= avcodec_alloc_context3(encoder);
    if (!encoder_ctx){
        return -1;
    }

    encoder_ctx->time_base=ctx->streams[0]->time_base;
    encoder_ctx->width=decoder_ctx->width;
    encoder_ctx->height=decoder_ctx->height;
    encoder_ctx->frame_size=decoder_ctx->frame_size;
    encoder_ctx->pix_fmt=decoder_ctx->pix_fmt;

    encoder_ctx->gop_size=decoder_ctx->gop_size;
    encoder_ctx->keyint_min=decoder_ctx->keyint_min;
    encoder_ctx->has_b_frames=decoder_ctx->has_b_frames;
    encoder_ctx->max_b_frames=decoder_ctx->max_b_frames;
    encoder_ctx->refs=decoder_ctx->refs;




    if(avcodec_open2(encoder_ctx,encoder,NULL)){
        return -1;
    }


    av_log(NULL,AV_LOG_INFO,"输入编码 %s/%s\n",decoder->name,decoder->long_name);

    av_log(NULL,AV_LOG_INFO,"输入%dx%d,fps %d\n",decoder_ctx->width,decoder_ctx->height,decoder_ctx->framerate);


    AVStream* new_stream=avformat_new_stream(out_ctx,NULL);
    if(avcodec_parameters_from_context(new_stream->codecpar,encoder_ctx)<0){
        return -1;
    }



    if(avformat_write_header(out_ctx,NULL)<0){
        return -1;
    }
    av_dump_format(out_ctx,0,outfile,1);

    AVPacket pkt;
    AVPacket new_pkt;
    while (1) {
        av_init_packet(&pkt);

        if(av_read_frame(ctx,&pkt)<0){
            break;
        }

        if (pkt.stream_index!=0){
            continue;
        }



        int ret=decode(decoder_ctx,frame,&pkt);
        if (ret<0){
            return -1;
        } else if (ret>0){
            printf(" Frame PTS %10s ,  %d\n"
                    , av_ts2timestr(frame->pts,time_base)
                    ,frame->key_frame);

            ret=encode(encoder_ctx,&new_pkt,frame,out_ctx);
            if (ret<0){
                return -1;
            }
        }

        av_packet_unref(&pkt);

    }

    //flush decoder
    while (1){
        int ret=decode(decoder_ctx,frame,NULL);
        if (ret>0){
            printf(" Frame PTS %10s ,  %d\n"
                    , av_ts2timestr(frame->pts,time_base)
                    ,frame->key_frame);
            encode(encoder_ctx,&new_pkt,frame,out_ctx);
        } else{
            break;
        }
    }


    encode(encoder_ctx,&new_pkt,NULL,out_ctx);

    av_frame_free(&frame);



    av_write_trailer(out_ctx);
    if (!no_need_provide_io_context)
        avio_closep(&out_ctx->pb);

    avformat_free_context(out_ctx);
    avformat_free_context(ctx);





}

AVCodecContext* global_decoder_codec_ctx;
AVRational global_time_base;
int packageView(const char * infile,std::vector<AVFrame*>& frames){
    int lines=0;
    AVFormatContext *ctx=NULL;

    av_register_all();
    av_log_set_level(AV_LOG_WARNING);
    if(avformat_open_input(&ctx,infile,NULL,NULL)<0){
        return -1;
    }

    if(avformat_find_stream_info(ctx,NULL)<0){
        return -1;
    }


    AVRational* time_base=&ctx->streams[0]->time_base;
    av_log(NULL,AV_LOG_INFO,"流的时间基 %d/%d\n",time_base->num,time_base->den);





    //////////////////////////////////////////////////////////////////////
    AVCodec * decoder=avcodec_find_decoder(ctx->streams[0]->codecpar->codec_id);

    if (!decoder){
        return -1;
    }

    AVCodecContext* decoder_ctx= avcodec_alloc_context3(decoder);
    global_decoder_codec_ctx=decoder_ctx;
    if (!decoder_ctx){
        return -1;
    }
    avcodec_parameters_to_context(decoder_ctx,ctx->streams[0]->codecpar);


    if(avcodec_open2(decoder_ctx,decoder,NULL)){
        return -1;
    }

    global_time_base=ctx->streams[0]->time_base;
    AVPacket pkt;

    while (1) {
        av_init_packet(&pkt);

        if(av_read_frame(ctx,&pkt)<0){
            break;
        }

        if (pkt.stream_index!=0){
            continue;
        }




//        printf("Packet PTS %10s,DPT %10s,时长 %10s,  %c  大小 %d,\n"
//                , av_ts2timestr(pkt.pts,time_base)
//                , av_ts2timestr(pkt.dts,time_base)
//                , av_ts2timestr(pkt.duration,time_base),key,pkt.size);
        AVFrame * frame=av_frame_alloc();

        int ret=decode(decoder_ctx,frame,&pkt);
        if (ret<0){
            return -1;
        } else if (ret>0){
            printf(" Frame PTS %10s,pos %5d\n"
                    , av_ts2timestr(frame->pts,time_base)
                    ,lines++);
            frames.emplace_back(frame);
        }
        av_packet_unref(&pkt);
    }

    //flush decoder
    while (1){
        AVFrame * frame=av_frame_alloc();
        int ret=decode(decoder_ctx,frame,NULL);
        if (ret>0){
            printf(" Frame PTS %10s,pos %5d\n"
                    , av_ts2timestr(frame->pts,time_base)
                    ,lines++);
            frames.emplace_back(frame);
        } else{
            break;
        }
    }



}

int packageView(const char * infile){
    int lines=0;
    AVFormatContext *ctx=NULL;

    av_register_all();
    av_log_set_level(AV_LOG_INFO);
    if(avformat_open_input(&ctx,infile,NULL,NULL)<0){
        return -1;
    }

    if(avformat_find_stream_info(ctx,NULL)<0){
        return -1;
    }


    AVRational* time_base=&ctx->streams[0]->time_base;
    av_log(NULL,AV_LOG_INFO,"流的时间基 %d/%d\n",time_base->num,time_base->den);


    AVFrame * frame=av_frame_alloc();

    int cnt=0;

    //////////////////////////////////////////////////////////////////////
    AVCodec * decoder=avcodec_find_decoder(ctx->streams[0]->codecpar->codec_id);

    if (!decoder){
        return -1;
    }

    AVCodecContext* decoder_ctx= avcodec_alloc_context3(decoder);
    if (!decoder_ctx){
        return -1;
    }

    avcodec_parameters_to_context(decoder_ctx,ctx->streams[0]->codecpar);


    if(avcodec_open2(decoder_ctx,decoder,NULL)){
        return -1;
    }

    AVPacket pkt;

    int pos=0;
    while (1) {
        av_init_packet(&pkt);

        if(av_read_frame(ctx,&pkt)<0){
            break;
        }

        if (pkt.stream_index!=0){
            continue;
        }

        char key=' ';
        if(AV_PKT_FLAG_KEY&pkt.flags){
            key='Y';
        }

//        printf("Pos %3d Packet PTS %10s,DPT %10s,时长 %10s,  %c  大小 %d,\n"
//                ,pos++
//                , av_ts2timestr(pkt.pts,time_base)
//                , av_ts2timestr(pkt.dts,time_base)
//                , av_ts2timestr(pkt.duration,time_base),key,pkt.size);
        printf("Pos %3d Packet PTS %10d,DPT %10d,时长 %10d,  %c  大小 %d,\n"
                ,pos++
                , (pkt.pts)
                , (pkt.dts)
                , (pkt.duration),key,pkt.size);
        cnt++;
//        int ret=decode(decoder_ctx,frame,&pkt);
//        if (ret<0){
//            return -1;
//        } else if (ret>0){
//            printf(" Frame PTS %10s,pos %5d\n"
//                    , av_ts2timestr(frame->pts,time_base)
//                    ,lines++);
//        }
        av_packet_unref(&pkt);
    }
    printf("总共数据包%d\n",cnt);
    //flush decoder
    while (1){
        int ret=decode(decoder_ctx,frame,NULL);
        if (ret>0){
            printf(" Frame PTS %10s,pos %5d\n"
                    , av_ts2timestr(frame->pts,time_base)
                    ,lines++);
        } else{
            break;
        }
    }
}



int outPackage(std::vector<AVFrame*>&frames,const char * outfile){
    AVFormatContext *out_ctx=NULL;
    avformat_alloc_output_context2(&out_ctx, NULL, NULL, outfile);
    int no_need_provide_io_context =out_ctx->oformat->flags & AVFMT_NOFILE;

    if (!no_need_provide_io_context) {
        int ret = avio_open(&out_ctx->pb, outfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_ctx);
            return -1;
        }
    }


    if (!out_ctx){
        return -1;
    }

    av_register_all();
    av_log_set_level(AV_LOG_INFO);


    AVCodec *encoder= avcodec_find_encoder_by_name("libx264");
    if (!encoder){
        return -1;
    }
    AVCodecContext* encoder_ctx= avcodec_alloc_context3(encoder);
    if (!encoder_ctx){
        return -1;
    }


    encoder_ctx->time_base=global_time_base;

    encoder_ctx->framerate=(AVRational){global_time_base.den,global_time_base.num};
    encoder_ctx->width=global_decoder_codec_ctx->width;
    encoder_ctx->height=global_decoder_codec_ctx->height;
    encoder_ctx->frame_size=global_decoder_codec_ctx->frame_size;
    encoder_ctx->pix_fmt=global_decoder_codec_ctx->pix_fmt;



    if(avcodec_open2(encoder_ctx,encoder,NULL)){
        return -1;
    }
    AVStream* new_stream=avformat_new_stream(out_ctx,NULL);
    if(avcodec_parameters_from_context(new_stream->codecpar,encoder_ctx)<0){
        return -1;
    }


    if(avformat_write_header(out_ctx,NULL)<0){
        return -1;
    }
    av_dump_format(out_ctx,0,outfile,1);

    AVPacket new_pkt;
    int total=0;
    int success=0;
    for(AVFrame* frame:frames){
        int ret=encode(encoder_ctx,&new_pkt,frame,out_ctx);
        if (ret<0){
            exit(1);
        }
        total++;
        av_packet_unref(&new_pkt);
    }





    while (1){
        int ret=encode(encoder_ctx,&new_pkt,NULL,out_ctx);
        if (ret>0){
            printf("New Pack PTS %10s,DPT %10s,时长 %10s,  大小 %d,\n"
                    , av_ts2timestr(new_pkt.pts,&global_decoder_codec_ctx->time_base)
                    , av_ts2timestr(new_pkt.dts,&global_decoder_codec_ctx->time_base)
                    , av_ts2timestr(new_pkt.duration,&global_decoder_codec_ctx->time_base),new_pkt.size);
            av_write_frame(out_ctx,&new_pkt);

        } else{
            break;
        }
        av_packet_unref(&new_pkt);
    }


    av_write_trailer(out_ctx);
    if (!no_need_provide_io_context)
        avio_closep(&out_ctx->pb);

    avformat_free_context(out_ctx);


}
int main(){
//    const char * infile="../file/hls/abc-1692768387558-3.ts";
//    const char * outfile="../file/hls/zxk-h254-2.ts";
//
//    transcode(infile,outfile);

    std::string src=".";
    std::string dest="/Users/zhanggxk/project/lal/lal_record/hls/zhangxk";

    std::vector<std::string> filenames;


//    packageView("/Users/zhanggxk/Desktop/error/39671f8ce7910af5d3faa5c0f073ce26-1.avi");
    packageView("/Users/zhanggxk/Desktop/测试视频/dahua.h264");
    std::vector<AVFrame *> all_frames;
    for(const std::string& f:filenames){
        std::string target=dest+"/"+f;
        std::string source=src+"/"+f;

//        transcode(source.c_str(),target.c_str());
//        packageView(target.c_str());
//        packageView(source.c_str(),all_frames);
    }
//
//    for(const std::string& f:filenames){
//        std::string target=dest+"/"+f;
//        std::string source=src+"/"+f;
////
//////        transcode(source.c_str(),target.c_str());
//////        packageView(target.c_str());
//        outPackage(all_frames,target.c_str());
//    }

}