#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavutil/log.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif





//clang -o ffmpeg_dir ffmpeg_dir.c -I /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/include -L /Users/zzzlllll/Downloads/ffmpeg-4.3.2/build/lib -lavutil -lavformat
// `pkgconfig --libs libavutil libavformat`

//AVIODirContext,AVIODirEntry

//avio_open_dir
//avio_close_dir
//avio_read_dir
//avio_free_directory_entry
int main(int argc,char* argv[]) {
    av_log_set_level(AV_LOG_INFO);

    AVIODirContext *ctx=NULL;
    int ret=0;

    char *filepath = "./";
    ret=avio_open_dir(&ctx,filepath,NULL);

    if(ret<0){
        goto end;
    }


    while (1){
        AVIODirEntry *entry=NULL;
        ret=avio_read_dir(ctx,&entry);

        if(ret<0){
            goto end;
        }
        //目录的末尾
        if(!entry){
            break;
        }
        //3-dir,5-link 7-file
        const char *type=entry->type==3?"d":entry->type==5?"l":"f";
        av_log(NULL,AV_LOG_INFO,"%s %20s %12lld\n",type,entry->name,entry->size);
        avio_free_directory_entry(&entry);
    }


    end:
    avio_close_dir(&ctx);
    if(ret<0){
        av_log(NULL, AV_LOG_ERROR, "Error:%s", av_err2str(ret));
    }
    return ret;
}