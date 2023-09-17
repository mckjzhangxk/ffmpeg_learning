
#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <SDL.h>

#ifdef __cplusplus
}
#endif

#include <vector>

#define Player_AUDIO_CHANNEL 1
#define Player_AUDIO_SAMPLE_RATE 48000

SDL_Renderer * renderer=NULL;


class PacketQueue{
public:
    PacketQueue():m_duration(0),m_size(0){
        m_mutex=SDL_CreateMutex();
        m_cond=SDL_CreateCond();
    }


    ~PacketQueue(){
        flush();
        SDL_DestroyCond(m_cond);
        SDL_DestroyMutex(m_mutex);
    }
    int enqueue(AVPacket* pkt){
        AVPacket* new_pkt=av_packet_alloc();

        if(!new_pkt){
            av_packet_unref(pkt);
            return -1;
        }
        //这个方法，其实是把pkt的内部字段都释放掉了
        av_packet_move_ref(new_pkt,pkt);

        SDL_LockMutex(m_mutex);
        m_size+=new_pkt->size;
        m_duration+=new_pkt->duration;
        m_data.emplace_back(new_pkt);

        SDL_CondSignal(m_cond);//发送信号给对端，这个函数应该是不阻塞的
        SDL_UnlockMutex(m_mutex);
    }
    //block=1,表示收不到新pkt不反回
    int dequeue(AVPacket* pkt,bool block){
        int ret=-1;
        SDL_LockMutex(m_mutex);

        while (1){
            if (!m_data.empty()){
                AVPacket* p=m_data[0];
                m_size-=p->size;
                m_duration-=p->duration;
                //这个方法，其实是把pkt的内部字段都释放掉了,所以没有必要再调用av_package_unref()
                av_packet_move_ref(pkt,p);
                m_data.erase(m_data.begin());
                ret=1;
                break;
            }
            if (!block){
                ret=0;
                break;
            } else{
                SDL_CondWait(m_cond,m_mutex);//解锁m_mutex，然后阻塞等待信号量，收到信号时，重新再去加锁
            }
        }

        SDL_UnlockMutex(m_mutex);

        return ret;
    }

    void flush(){
        SDL_LockMutex(m_mutex);

        for (int i = 0; i <m_data.size() ; ++i) {
            av_packet_free(&m_data[i]);
        }
        m_data.clear();
        m_size=0;
        m_duration=0;
        SDL_UnlockMutex(m_mutex);
    }
private:
    std::vector<AVPacket*> m_data;
    int m_size;
    int64_t m_duration;

    SDL_mutex *m_mutex;
    SDL_cond * m_cond;
};


int get_channel_layout_from_channels(int n){
    switch (n) {
        case 1:
            return AV_CH_LAYOUT_MONO;
        case 2:
            return AV_CH_LAYOUT_STEREO;
    }
}
class Converter{
public:
    Converter(AVCodecContext* ctx){
        m_sample_rate=ctx->sample_rate;
        m_swrContext=swr_alloc();

        av_opt_set_int(m_swrContext, "in_channel_layout", ctx->channel_layout, 0);
        av_opt_set_int(m_swrContext, "in_sample_rate", ctx->sample_rate, 0);
        av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", ctx->sample_fmt, 0);
        av_opt_set_int(m_swrContext, "out_channel_layout", get_channel_layout_from_channels(Player_AUDIO_CHANNEL), 0); // 目标通道布局（立体声）
        av_opt_set_int(m_swrContext, "out_sample_rate", Player_AUDIO_SAMPLE_RATE, 0); // 目标采样率
        av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0); // 目标采样格式（16位有符号整数）

        if(!m_swrContext){
            av_log(m_swrContext, AV_LOG_ERROR, "无法创建重采样上下文");
            return;
        }

        if (swr_init(m_swrContext) < 0){
            av_log(m_swrContext, AV_LOG_ERROR, "无法初始化重采样上下文");
            return;
        }
    }

    AVFrame* convert(AVFrame* frame,int& size_per_channel){
        AVFrame *out_frame=av_frame_alloc();

        out_frame->channel_layout= get_channel_layout_from_channels(Player_AUDIO_CHANNEL);
        out_frame->channels=Player_AUDIO_CHANNEL;
        out_frame->nb_samples=frame->nb_samples*Player_AUDIO_SAMPLE_RATE/ m_sample_rate;
        out_frame->format=AV_SAMPLE_FMT_S16;

        int result=av_frame_get_buffer(out_frame,1);
        if(result<0){
            av_log(frame,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
            return NULL;
        }

        size_per_channel=swr_convert(m_swrContext,
                           (uint8_t **)out_frame->data, out_frame->nb_samples,
                           (const uint8_t **)frame->data, frame->nb_samples);
        size_per_channel=size_per_channel* av_get_bytes_per_sample((AVSampleFormat)out_frame->format);
        return out_frame;
    }
    ~Converter(){
        if (m_swrContext){
            swr_free(&m_swrContext);
        }
    }
private:
    SwrContext* m_swrContext;
    int m_sample_rate;
};

typedef struct _VideoState {
    AVCodecContext *video_avctx;  //解码上下文
    AVPacket       *pkt;
    AVFrame        *vframe;

    AVCodecContext *audio_avctx;  //解码上下文
    AVFrame        *aframe;
    SDL_Texture    *texture;
    PacketQueue    queue;

    Converter* converter;

}VideoState;


//从FIFO中解出尽可能不超过data_size的音频数据，并返回实际的音频数据大小
//返回 -1 表示解码失败
int audio_decode(VideoState *is, Uint8* data,int data_size) {

    AVPacket packet;

    //非阻塞的方法
    int ret=is->queue.dequeue(&packet,1);

    if (ret){
        ret = avcodec_send_packet(is->audio_avctx, &packet);
        if(ret < 0) {
            av_packet_unref(&packet);
            av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
            return -1;
        }
        int readn=0;
        while( ret >= 0){
            ret = avcodec_receive_frame(is->audio_avctx, is->aframe);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            } else if( ret < 0) {
                av_packet_unref(&packet);
                return -1;
            }

            int frame_size; //没有考虑双通道
            AVFrame* play_frame=is->converter->convert(is->aframe,frame_size);
            av_frame_unref(is->aframe);


            //把数据赋值给data

//            frame_size= av_samples_get_buffer_size(NULL,is->aframe->channels,is->aframe->nb_samples,is->audio_avctx->sample_fmt,0);
            memcpy(data+readn,play_frame->data[0],frame_size);

            readn+=frame_size;
            av_frame_free(&play_frame);
        }
        av_packet_unref(&packet);
        return readn;
    } else{
        return 0;
    }

}
#define BLOCK_SIZE 1024*1024
//userdata：透传数据
void read_audio_callback(void *userdata, Uint8 * stream,int len){
    static Uint8 AUDIO_DATA[BLOCK_SIZE];
    int AUDIO_SIZE=0; //AUDIO_DATA中有效的音频数据
    int AUDIO_INDEX=0;//下次拷贝时候从AUDIO_DATA的索引

    VideoState *is=(VideoState*)userdata;

    while (len>0){
        if (AUDIO_INDEX>=AUDIO_SIZE){
            //decode Audio Data
            int readn=audio_decode(is,AUDIO_DATA,BLOCK_SIZE);
            AUDIO_INDEX=0;
            AUDIO_SIZE=readn;
        }
        int remain=AUDIO_SIZE-AUDIO_INDEX;
        if (remain<=0){
            memset(stream,0,len);
            len=0;
        } else{
            int copy_num=len>=remain?remain:len;
            memcpy(stream,AUDIO_DATA+AUDIO_INDEX,copy_num);

            AUDIO_INDEX+=copy_num;
            stream+=copy_num;
            len-=copy_num;
        }

    }
}



static void render(VideoState *is){

    SDL_UpdateYUVTexture(is->texture,
                         NULL,
                         is->vframe->data[0], is->vframe->linesize[0],
                         is->vframe->data[1], is->vframe->linesize[1],
                         is->vframe->data[2], is->vframe->linesize[2]);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, is->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


int video_decode(VideoState *is){
    int ret = -1;

    ret = avcodec_send_packet(is->video_avctx, is->pkt);
    if(ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto __OUT;
    }

    while( ret >= 0){
        ret = avcodec_receive_frame(is->video_avctx, is->vframe);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            ret = 0;
            goto __OUT;
        } else if( ret < 0) {
            ret = -1; //退出程序
            goto __OUT;
        }
        render(is);
    }
    __OUT:
    return ret;
}

//播放一个简单的视频
//解码的帧交给SDL显示

//问题:
// 1.窗口比例与视频比例不符
// 2.没有按照FPS播放，播放过快
int main(int  argc,char *argv[]){

    av_log_set_level(AV_LOG_DEBUG);

    //1. 判断输入参数
    if(argc < 2){ //argv[0], simpleplayer, argv[1] src
        av_log(NULL, AV_LOG_INFO, "usage simple_player2  a.mp4\n");
        exit(-1);
    }

    const char *src = argv[1];

    //2. 初始化SDL，并创建窗口和Render
    //2.1
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    //2.2 creat window from SDL

    SDL_Window *win = SDL_CreateWindow("Simple Player",
                           SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                           0, 0,
                           SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(win, -1, 0);



    //3. 打开多媒体文件，并获得流信息
    int ret=0;
    AVFormatContext *fmtCtx=NULL;
    if((ret = avformat_open_input(&fmtCtx, src, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }

    if((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }

    //4. 查找最好的视频流
    int v_idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(v_idx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include video stream!\n");
        return -1;
    }

    int a_idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(a_idx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include audio stream!\n");
        return -1;
    }

    //5. 根据流中的codec_id, 获得解码器
    AVStream *v_inStream = fmtCtx->streams[v_idx];
    const AVCodec *v_dec = avcodec_find_decoder(v_inStream->codecpar->codec_id);
    if(!v_dec){
        av_log(NULL, AV_LOG_ERROR, "Could not find libx264 Codec");
        return -1;
    }

    AVStream *a_inStream = fmtCtx->streams[a_idx];
    const AVCodec *a_dec = avcodec_find_decoder(a_inStream->codecpar->codec_id);
    if(!a_dec){
        av_log(NULL, AV_LOG_ERROR, "Could not find libx264 Codec");
        return -1;
    }
    //6. 创建解码器上下文

    AVCodecContext * v_ctx = avcodec_alloc_context3(v_dec);
    if(!v_ctx){
        av_log(NULL, AV_LOG_ERROR, "NO MEMRORY\n");
        return -1;
    }

    AVCodecContext * a_ctx = avcodec_alloc_context3(a_dec);
    if(!a_ctx){
        av_log(NULL, AV_LOG_ERROR, "NO MEMRORY\n");
        return -1;
    }
    //7. 从视频流中拷贝解码器参数到解码器上文中
    ret = avcodec_parameters_to_context(v_ctx, v_inStream->codecpar);
    if(ret < 0){
        av_log(v_ctx, AV_LOG_ERROR, "Could not copyt codecpar to codec ctx!\n");
        return -1;
    }

    ret = avcodec_parameters_to_context(a_ctx, a_inStream->codecpar);
    if(ret < 0){
        av_log(v_ctx, AV_LOG_ERROR, "Could not copyt codecpar to codec ctx!\n");
        return -1;
    }

    //8. 绑定解码器上下文
    ret = avcodec_open2(v_ctx, v_dec , NULL);
    if(ret < 0) {
        av_log(v_ctx, AV_LOG_ERROR, "Don't open codec: %s \n", av_err2str(ret));
        return -1;
    }

    ret = avcodec_open2(a_ctx, a_dec , NULL);
    if(ret < 0) {
        av_log(v_ctx, AV_LOG_ERROR, "Don't open codec: %s \n", av_err2str(ret));
        return -1;
    }


    //9.1. 根据视频的宽/高创建纹理
    int video_width = v_ctx->width;
    int video_height = v_ctx->height;

    int w_width=1280;
    SDL_SetWindowSize(win, w_width,video_height*w_width/video_width);


    SDL_Texture *  texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_IYUV,
                                SDL_TEXTUREACCESS_STREAMING,
                                video_width,
                                video_height);



    AVPacket  *pkt=av_packet_alloc();


    Converter* converter=new Converter(a_ctx);
    //分配，并且都置0
    VideoState *is = new VideoState;
    is->vframe=av_frame_alloc();
    is->aframe=av_frame_alloc();
    is->texture=texture;
    is->pkt=pkt;
    is->video_avctx=v_ctx;
    is->audio_avctx=a_ctx;
    is->converter=converter;
    //9.2设置SDL 音频设备
    SDL_AudioSpec param;
    //音频三要素
    param.channels=Player_AUDIO_CHANNEL;
    param.format=AUDIO_S16SYS;//有符号的16位
    param.freq=Player_AUDIO_SAMPLE_RATE;
    param.callback=read_audio_callback;
    param.userdata=is;//透传参数
    param.silence=0; //表示0 代表没有声音
    param.samples=1024;//我理解这个 是声卡采集的buf大小

    if(SDL_OpenAudio(&param,NULL)){
        SDL_Log("无法打开Audio设备\n");
        return  -1;
    }


    //play audio
    SDL_PauseAudio(0);

    //10. 从多媒体文件中读取数据，进行解码
    while(av_read_frame(fmtCtx, pkt) >= 0) {
        if(pkt->stream_index == v_idx) {
            //11. 对解码后的视频帧进行渲染
            video_decode(is);
        } else if (pkt->stream_index==a_idx){
            is->queue.enqueue(pkt);
        } else{
            av_packet_unref(pkt);
        }
        //12. 处理SDL事件
        SDL_Event event;
        SDL_PollEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
                goto __END;
                break;
            default:
                break;
        }
        av_packet_unref(pkt);
    }

    is->pkt=NULL;
    video_decode(is);

__END:
    //13. 收尾，释放资源
    if(is->vframe){
        av_frame_free(&is->vframe);
    }
    if(is->aframe){
        av_frame_free(&is->aframe);
    }

    if(pkt){
        av_packet_free(&pkt);
    }

    if(v_ctx){
        avcodec_free_context(&v_ctx);
    }
    if(a_ctx){
        avcodec_free_context(&a_ctx);
    }
    if(fmtCtx){
        avformat_close_input(&fmtCtx);
    }

    if(win){
        SDL_DestroyWindow(win);
    }

    if(renderer){
        SDL_DestroyRenderer(renderer);
    }

    if(texture){
        SDL_DestroyTexture(texture);
    }

    if (is){
        delete is;
    }
    SDL_CloseAudio();
    SDL_Quit();
    return 0;
}