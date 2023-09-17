#include "libavformat/avformat.h"
#include "libavutil/avutil.h"



int main(int argc,char* argv[]){
    if(argc<3){
        av_log(NULL,AV_LOG_WARNING,"Usage:copy inputfile outputfile");
        return -1;
    }
    AVFormatContext * inFmt,*outFmt;
    AVPacket packet;
    int ret=0;
    int selectIndex=1;

    const char* inputFile=argv[1];
    char* outFile[255];
    sprintf(outFile,"%s-%d.flv",argv[2],selectIndex);
    av_log(NULL,AV_LOG_ERROR,outFile);
    ret=avformat_open_input(&inFmt,inputFile,NULL,NULL);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"Could not open file %s",inputFile);
        goto end;
    }
    //可以根据包 字段计算 有多少stream
    if ((ret = avformat_find_stream_info(inFmt, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Failed to retrieve input stream information");
        goto end;
    }

    ret=avformat_alloc_output_context2(&outFmt,NULL,NULL,outFile);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"Could not open file %s",outFile);
        goto end;
    }
    av_dump_format(inFmt,0,inputFile,0);


    AVStream** out_streams=av_malloc_array(inFmt->nb_streams,sizeof (AVStream*));

    for (int i = 0; i <inFmt->nb_streams; ++i) {
        if(selectIndex!=i) continue;
        AVStream* stream=inFmt->streams[i];
        AVStream* out_stream=avformat_new_stream(outFmt,NULL);
        avcodec_parameters_copy(out_stream->codecpar,stream->codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(outFmt,1,outFile,1);
    AVFormatContext* formatContext=outFmt->oformat;

    if(!(formatContext->flags&AVFMT_NOFILE)){
        ret=avio_open(&outFmt->pb,outFile,AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR,"Could not open output file '%s'", outFile);
            goto end;
        }
    }

    if(ret<0){
        av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", outFile);
        goto end;
    }
    avformat_write_header(outFmt,NULL);

    av_init_packet(&packet);
    while (!av_read_frame(inFmt,&packet)){
        if(selectIndex!=packet.stream_index) continue;
        AVStream* stream=inFmt->streams[packet.stream_index];
        AVStream* out_stream=outFmt->streams[0];//out_streams[packet.stream_index];

        packet.pts=av_rescale_q(packet.pts,stream->time_base,out_stream->time_base);
        packet.dts=av_rescale_q(packet.dts,stream->time_base,out_stream->time_base);
        packet.duration=av_rescale_q(packet.duration,stream->time_base,out_stream->time_base);

        packet.stream_index=0;
        packet.pos=-1;

        av_interleaved_write_frame(outFmt,&packet);
        av_packet_unref(&packet);
    }
    av_write_trailer(outFmt);

    end:
    avformat_close_input(&inFmt);

}