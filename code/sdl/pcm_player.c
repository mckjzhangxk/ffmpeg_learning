#include "SDL.h"

#define CHANNELS 2
#define FREQ  44100
#define BLOCK_SIZE 4096000*4
char AUDIO_DATA[BLOCK_SIZE];



//userdata：透传数据
void read_audio_callback(void *userdata, Uint8 * stream,int len){

//    SDL_MixAudio()
    FILE *fd=(FILE *)userdata;
    int readn=fread(AUDIO_DATA,1,len,fd);

    if (readn<=0){
        SDL_Event event;
        event.type=SDL_QUIT;
        SDL_PushEvent(&event);
    }
    //以下2个方法等效。

//    SDL_memset(stream,0,len);
//    SDL_MixAudio(stream,AUDIO_DATA,readn,SDL_MIX_MAXVOLUME);

    memset(stream,0,len);
    memcpy(stream,AUDIO_DATA,readn);
}

//打开音频设备-》设置音频参数-》喂数据->播放视频-》关闭设备

//1;声卡的数据 是向你的回调函数 要数据
//2;声卡的数据大小由 设置的音频参数决定。


//SDL_OpenAudio/SDL_CloseAudio
//SDL_PauseAudio(0),SDL_PauseAudio(1)
//SDL_MixAudio,混音函数，把数据交给声卡输出


//本程序利用read_audio.cpp的输出voice.pcm，经过重复采用的结果voice-new.pcm
//播放voice-new.pcm。

////数据源：
////ffmpeg -ar 48000 -f f32le -ac 1 -i  voice.pcm  -ar 44100 -f s16le -ac 2  voice-new.pcm
//// ffplay -ar 44100 -f s16le -ac 2 voice-new.pcm
int main(int  argc,char *argv[]){
    if (argc<2){
        return -1;
    }

    FILE *audio_fd = NULL;
    audio_fd = fopen(argv[1], "rb");
    if( !audio_fd ){
        fprintf(stderr, "Failed to open pcm file\n");
        return -1;
    }

    if(SDL_Init(SDL_INIT_AUDIO)){
        SDL_Log("无法初始化音频\n");
        return  -1;
    }



    SDL_AudioSpec param;
    //音频三要素
    param.channels=CHANNELS;
    param.format=AUDIO_S16SYS;//有符号的16位
    param.freq=FREQ;
    param.callback=read_audio_callback;
    param.userdata=audio_fd;//透传参数

    if(SDL_OpenAudio(&param,NULL)){
        SDL_Log("无法打开Audio设备\n");
        return  -1;
    }


    //play audio
    SDL_PauseAudio(0);

    int quit=1;

    do {
        SDL_Event event;
        //没有消息的时候阻塞，直到有消息唤起这个方法
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                quit=0;
                break;
        }
    } while (quit);


    SDL_CloseAudio();
    if (audio_fd){
        fclose(audio_fd);
    }

    SDL_Quit();
    return 0;
}