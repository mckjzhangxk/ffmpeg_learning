//
// Created by zzzlllll on 2021/12/5.
//
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"


typedef struct InputMetaData{
    char* filename;
    int videoIndex;
    int audioIndex;
    AVFormatContext* cxt;
    long offsetVideo;
    long offsetAudio;
} InputMetaData;

void getInput(InputMetaData **pContext, const char *url, int* result);
void getOutput(InputMetaData** metaOutout, const char *outfile, InputMetaData *metaInput,int* result);
void freeMeatData(int size, InputMetaData **pContext);
void concat(InputMetaData* inputMetaData,InputMetaData* outputMetaData);

int main(int argc, char* argv[]){
    int maxSize=argc-2,result=0;
    InputMetaData *inMeta,*outMeta;
    const char *outfile=argv[argc-1];
    if(maxSize<1){
        av_log(NULL,AV_LOG_WARNING,"usage:concat file1 file2 [fileN] outfile");
        return 0;
    }

    for (int i = 0; i <maxSize ; ++i) {
        getInput(&inMeta, argv[i+1], &result);
        if(result){
            if(i==0){
                goto end;
            }
            else continue;
        }
        if(i==0){
            getOutput(&outMeta, outfile, inMeta,&result);
            if(result) goto end;
            avformat_write_header(outMeta->cxt,NULL);
        }

        concat(inMeta, outMeta);
        av_log(NULL,AV_LOG_WARNING,"vOffset,aOffset: %ld,%ld\n",outMeta->offsetVideo,outMeta->offsetAudio);
    }
    av_write_trailer(outMeta->cxt);
    freeMeatData(1, &outMeta);

end:
    av_log(NULL,AV_LOG_INFO,"Finish");
}

void freeMeatData(int size, InputMetaData **pContext) {
    for (int i=0;i<size;i++){
        avformat_free_context(pContext[i]->cxt);
        free(pContext[i]);
    }

}

void getInput(InputMetaData **pContext, const char *url, int* result) {
        *result=1;
        AVFormatContext* inCxt=NULL;
        int ret=avformat_open_input(&inCxt,url,NULL,NULL);
        if(ret){
            av_log(NULL,AV_LOG_WARNING,"%s can't open Context",url);
            return;
        }
        ret=avformat_find_stream_info(inCxt,NULL);
        if(ret){
            av_log(NULL,AV_LOG_WARNING,"%s avformat_find_stream_info Error",url);
            return;
        }
        av_dump_format(inCxt,0,url,0);
        InputMetaData* metaData=malloc(sizeof (InputMetaData));
        metaData->offsetAudio=metaData->offsetVideo=0;
        metaData->filename=url;
        metaData->cxt=inCxt;
        metaData->audioIndex=av_find_best_stream(inCxt,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,NULL);
        metaData->videoIndex=av_find_best_stream(inCxt,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,NULL);
        *pContext=metaData;
        *result=0;
}

void getOutput(InputMetaData **metaOutout, const char* outfile, InputMetaData *metaInput,int* result) {
    *result=0;
    *metaOutout=malloc(sizeof (InputMetaData));

    InputMetaData* metaOut=*metaOutout;
    InputMetaData* metaIn=metaInput;
    AVFormatContext* inCxt=metaIn->cxt;
    AVFormatContext* outCxt=NULL;
    avformat_alloc_output_context2(&outCxt,NULL,NULL,outfile);


    metaOut->audioIndex=0;
    metaOut->videoIndex=1;
    metaOut->filename=outfile;
    metaOut->offsetVideo=0;
    metaOut->offsetAudio=13000;
    metaOut->cxt=outCxt;

    //audio
    AVStream* s=inCxt->streams[metaIn->audioIndex];
    AVStream* t=avformat_new_stream(outCxt,NULL);
    avcodec_parameters_copy(t->codecpar,s->codecpar);
    //video
    s=inCxt->streams[metaIn->videoIndex];
    t=avformat_new_stream(outCxt,NULL);
    avcodec_parameters_copy(t->codecpar,s->codecpar);

    if(!(metaOut->cxt->oformat->flags&AVFMT_NOFILE)){
        int ret=avio_open(&metaOut->cxt->pb,outfile,AVIO_FLAG_WRITE);
        *result=ret;
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avio_open Error");
            freeMeatData(1,&metaOut);
        }
    }
    av_dump_format(outCxt,0,outfile,1);


}

void concat(InputMetaData* inMetas, InputMetaData* outMeta) {
    AVFormatContext * inCxt=inMetas->cxt;
    AVFormatContext * outCxt=outMeta->cxt;

    AVPacket* packet=av_packet_alloc();
    av_init_packet(packet);


    long audioDuration=0,videoDuration=0;
    while (!av_read_frame(inCxt,packet)){
        int streamIdx=packet->stream_index;
        int targetInx=(streamIdx==inMetas->audioIndex)?outMeta->audioIndex:outMeta->videoIndex;
        int offset=(streamIdx==inMetas->audioIndex)?outMeta->offsetAudio:outMeta->offsetVideo;
        //这里巧妙的使用了指针，更新了视频和音频的总时长
        long *duration=(streamIdx==inMetas->audioIndex)?&audioDuration:&videoDuration;

        AVStream* stream=inCxt->streams[streamIdx];
        AVStream* outStream=outCxt->streams[targetInx];

        packet->pts=offset+av_rescale_q_rnd(packet->pts,stream->time_base,outStream->time_base,AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet->dts=offset+av_rescale_q_rnd(packet->dts,stream->time_base,outStream->time_base,AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet->duration=av_rescale_q_rnd(packet->duration,stream->time_base,outStream->time_base,AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        packet->stream_index=targetInx;


        *duration=packet->pts+packet->duration;
        av_interleaved_write_frame(outCxt,packet);

        av_packet_unref(packet);
    }

    outMeta->offsetVideo=videoDuration;
    outMeta->offsetAudio=audioDuration;

    freeMeatData(1,&inMetas);
}
