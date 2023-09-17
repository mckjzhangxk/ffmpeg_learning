#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

AVFormatContext* getInput(char * inputPath,int index);

int main(int argc,char* argv[]){
    AVFormatContext* inputCtxs[10],*outFmtCtx;
    AVPacket packet;
    int ret=0,mixSize=0;
    const char* outputFile=argv[argc-1];

    if(argc<3){
        av_log(NULL,AV_LOG_WARNING,"Usage merge file1 file2 ... outfile");
        return 0;
    }
    mixSize=argc-2;

    av_register_all();
    ret=avformat_alloc_output_context2(&outFmtCtx,NULL,NULL,outputFile);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"avformat_alloc_output_context2 Error");
        goto end;
    }

    for(int i=1;i<argc-1;i++){
        AVFormatContext * inpFmtCxt=getInput(argv[i],i);
        if(inpFmtCxt==NULL){
            mixSize--;
            continue;
        }
        inputCtxs[i-1]=inpFmtCxt;

        AVStream* inStream=inpFmtCxt->streams[0];
        AVStream* outStream=avformat_new_stream(outFmtCtx,NULL);

        avcodec_parameters_copy(outStream->codecpar,inStream->codecpar);
//        outStream->codecpar->codec_tag=0;
    }

    if(!(outFmtCtx->oformat->flags&AVFMT_NOFILE)){
        ret=avio_open(&outFmtCtx->pb,outputFile,AVIO_FLAG_WRITE);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avio_open Error");
            goto end;
        }
    }

    av_dump_format(outFmtCtx,0,outputFile,1);

    avformat_write_header(outFmtCtx,NULL);
    int hasRemaining=1;
    while (hasRemaining){
        hasRemaining=0;
        for (int i = 0; i < mixSize; ++i) {
            AVFormatContext * inpFmtCxt=inputCtxs[i];

            ret=av_read_frame(inpFmtCxt,&packet);
            if(ret!=0) continue;
            hasRemaining=1;

            AVStream* inStream=inpFmtCxt->streams[0];
            AVStream* outStream=outFmtCtx->streams[i];

            packet.stream_index=i;
            packet.pts=av_rescale_q(packet.pts,inStream->time_base,outStream->time_base);
            packet.dts=av_rescale_q(packet.dts,inStream->time_base,outStream->time_base);
            packet.duration=av_rescale_q(packet.duration,inStream->time_base,outStream->time_base);
            packet.pos=-1;
            av_interleaved_write_frame(outFmtCtx,&packet);

            av_packet_unref(&packet);

        }
    }


    av_write_trailer(outFmtCtx);

    end:
    for (int i = 0; i <mixSize; ++i) {
        avformat_free_context(inputCtxs[i]);
    }

}


AVFormatContext* getInput(char * inputPath,int index){
    AVFormatContext* f;
    int ret=avformat_open_input(&f,inputPath,NULL,NULL);
    if(ret<0){
        return NULL;
    }
    ret=avformat_find_stream_info(f,NULL);
    if(ret<0){
        return NULL;
    }
    av_dump_format(f,index,inputPath,0);
    return f;
}