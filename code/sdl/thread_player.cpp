#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <SDL.h>

#ifdef __cplusplus
}
#endif

#include <vector>

#define THREAD_QUIT  (SDL_USEREVENT + 2)
#define VIDEO_LOAD_EVENT  (SDL_USEREVENT + 3)
#define VIDEO_NEW_FRAME  (SDL_USEREVENT + 4)
#define PLAY_END  (SDL_USEREVENT + 5)
#define Player_AUDIO_CHANNEL 1
#define Player_AUDIO_SAMPLE_RATE 44100
#define BLOCK_SIZE 1024*1024
//音频队列，视频队列


long long get_time_us(){
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC,&start_time);
    return start_time.tv_sec * 1000000LL + start_time.tv_nsec / 1000LL;

}
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
        return 0;
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



class FrameQueue{
public:
    FrameQueue(){
        m_mutex=SDL_CreateMutex();
        m_cond=SDL_CreateCond();
    }

    ~FrameQueue(){
        flush();
        SDL_DestroyCond(m_cond);
        SDL_DestroyMutex(m_mutex);
    }
    int enqueue(AVFrame * pkt){
        SDL_LockMutex(m_mutex);
        m_data.emplace_back(pkt);

        SDL_CondSignal(m_cond);//发送信号给对端，这个函数应该是不阻塞的
        SDL_UnlockMutex(m_mutex);
        return 0;
    }
    //block=1,表示收不到新pkt不反回
    AVFrame* dequeue(bool block){
        AVFrame* ret=NULL;
        SDL_LockMutex(m_mutex);

        while (1){
            if (!m_data.empty()){
                 ret=m_data[0];
                //这个方法，其实是把pkt的内部字段都释放掉了,所以没有必要再调用av_package_unref()
                m_data.erase(m_data.begin());
                break;
            }
            if (!block){
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
            av_frame_free(&m_data[i]);
        }
        m_data.clear();

        SDL_UnlockMutex(m_mutex);
    }
private:
    std::vector<AVFrame *> m_data;

    SDL_mutex *m_mutex;
    SDL_cond * m_cond;
};


//音频重采用工具类，把任意的输入音频 转换成s16,
//Player_AUDIO_CHANNEL,Player_AUDIO_SAMPLE_RATE的输出
class AudioConverter{
public:
    AudioConverter(AVCodecContext* ctx){
        m_sample_rate=ctx->sample_rate;
        m_swrContext=swr_alloc();

        av_opt_set_int(m_swrContext, "in_channel_layout", ctx->channel_layout, 0);
        av_opt_set_int(m_swrContext, "in_sample_rate", ctx->sample_rate, 0);
        av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", ctx->sample_fmt, 0);
        av_opt_set_int(m_swrContext, "out_channel_layout", Player_AUDIO_CHANNEL==1?AV_CH_LAYOUT_MONO:AV_CH_LAYOUT_STEREO, 0); // 目标通道布局（立体声）
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

        out_frame->channel_layout=Player_AUDIO_CHANNEL==1?AV_CH_LAYOUT_MONO:AV_CH_LAYOUT_STEREO;
        out_frame->channels=Player_AUDIO_CHANNEL;
        out_frame->nb_samples=frame->nb_samples*Player_AUDIO_SAMPLE_RATE/ m_sample_rate;
        out_frame->format=AV_SAMPLE_FMT_S16;

        int result=av_frame_get_buffer(out_frame,1);
        if(result<0){
            av_log(frame,AV_LOG_ERROR,"error:%s\n",av_err2str(result));
            return NULL;
        }

        int sample_per_channel=swr_convert(m_swrContext,
                                     (uint8_t **)out_frame->data, out_frame->nb_samples,
                                     (const uint8_t **)frame->data, frame->nb_samples);
        size_per_channel=sample_per_channel* av_get_bytes_per_sample((AVSampleFormat)out_frame->format);
        return out_frame;
    }
    ~AudioConverter(){
        if (m_swrContext){
            swr_free(&m_swrContext);
        }
    }
private:
    SwrContext* m_swrContext;
    int m_sample_rate;
};


typedef struct _VideoStream{
    char * infile;
    PacketQueue *video_queue;
    PacketQueue *audio_queue;
    FrameQueue * frame_queue;

    AVCodecContext *video_decoder;
    AVCodecContext *audio_decoder;


    long long start_time_ms;
    AVRational stream_time_base;
}VideoStream;
//从FIFO中解出尽可能不超过data_size的音频数据，并返回实际的音频数据大小
//返回 -1 表示解码失败
int audio_decode(VideoStream *is, Uint8* data,int data_size) {
    static AudioConverter converter(is->audio_decoder);
    static AVFrame *frame = av_frame_alloc();


    AVPacket packet;

    //非阻塞的方法
    int ret=is->audio_queue->dequeue(&packet,1);

    if (ret){
        ret = avcodec_send_packet(is->audio_decoder, &packet);
        if(ret < 0) {
            av_packet_unref(&packet);
            av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
            return -1;
        }
        av_packet_unref(&packet);

        int readn=0;

        while( ret >= 0){
            ret = avcodec_receive_frame(is->audio_decoder, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            } else if( ret < 0) {
                return -1;
            }

            int frame_size; //没有考虑双通道
            AVFrame* play_frame=converter.convert(frame,frame_size);
            av_frame_unref(frame);


            //把数据赋值给data

//            frame_size= av_samples_get_buffer_size(NULL,is->aframe->channels,is->aframe->nb_samples,is->audio_avctx->sample_fmt,0);
            memcpy(data+readn,play_frame->data[0],frame_size);

            readn+=frame_size;
            av_frame_free(&play_frame);
        }


        return readn;
    } else{
        return 0;
    }

}

int remux_thread (void *argv){
    void** p_argv=(void**)argv;

    int* quit=(int*)p_argv[0];
    VideoStream * is=(VideoStream *)p_argv[1];


    printf("=>解复用线程，输入文件 %s\n",is->infile);


    //3. 打开多媒体文件，并获得流信息
    int ret=0;
    AVFormatContext *fmtCtx=NULL;
    if((ret = avformat_open_input(&fmtCtx, is->infile, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }

    if((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }
    //4. 查找最好的视频流
    int v_idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(v_idx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include video stream!\n");
        exit(-1);
    }

    int a_idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(a_idx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include audio stream!\n");
        exit(-1);
    }
    av_dump_format(fmtCtx,0,is->infile,0);
    //5. 根据流中的codec_id, 获得解码器
    AVStream *v_inStream = fmtCtx->streams[v_idx];

    is->stream_time_base=v_inStream->time_base;
    const AVCodec *v_dec = avcodec_find_decoder(v_inStream->codecpar->codec_id);
    if(!v_dec){
        av_log(NULL, AV_LOG_ERROR, "Could not find video Codec");
        exit(-1);
    }

    AVStream *a_inStream = fmtCtx->streams[a_idx];
    const AVCodec *a_dec = avcodec_find_decoder(a_inStream->codecpar->codec_id);
    if(!a_dec){
        av_log(NULL, AV_LOG_ERROR, "Could not find audio Codec");
        exit(-1);
    }

    //6. 创建解码器上下文

    AVCodecContext * v_ctx = avcodec_alloc_context3(v_dec);
    if(!v_ctx){
        av_log(NULL, AV_LOG_ERROR, "NO MEMRORY\n");
        exit(-1);
    }

    AVCodecContext * a_ctx = avcodec_alloc_context3(a_dec);
    if(!a_ctx){
        av_log(NULL, AV_LOG_ERROR, "NO MEMRORY\n");
        exit(-1);
    }
    //7. 从视频流中拷贝解码器参数到解码器上文中
    ret = avcodec_parameters_to_context(v_ctx, v_inStream->codecpar);
    if(ret < 0){
        av_log(v_ctx, AV_LOG_ERROR, "Could not copyt codecpar to codec ctx!\n");
        exit(-1);
    }

    ret = avcodec_parameters_to_context(a_ctx, a_inStream->codecpar);
    if(ret < 0){
        av_log(v_ctx, AV_LOG_ERROR, "Could not copyt codecpar to codec ctx!\n");
        exit(-1);
    }
    //8. 绑定解码器上下文
    ret = avcodec_open2(v_ctx, v_dec , NULL);
    if(ret < 0) {
        av_log(v_ctx, AV_LOG_ERROR, "Don't open codec: %s \n", av_err2str(ret));
        exit(-1);
    }

    ret = avcodec_open2(a_ctx, a_dec , NULL);
    if(ret < 0) {
        av_log(v_ctx, AV_LOG_ERROR, "Don't open codec: %s \n", av_err2str(ret));
        exit(-1);
    }


    //9.1. 视频的长宽通知给主进程

    SDL_Event e;
    e.type=VIDEO_LOAD_EVENT;
    is->video_decoder=v_ctx;
    is->audio_decoder=a_ctx;
    SDL_PushEvent(&e);


    // 计算从某个点开始的时间戳（以毫秒为单位）

    is->start_time_ms = get_time_us();

    AVPacket pkt;

    while (!*quit && av_read_frame(fmtCtx, &pkt) >= 0){
        if(pkt.stream_index == v_idx) {
            is->video_queue->enqueue(&pkt);
        } else if (pkt.stream_index==a_idx){
            is->audio_queue->enqueue(&pkt);
        } else{
            av_packet_unref(&pkt);
        }

    }

    e.type=THREAD_QUIT;
    SDL_PushEvent(&e);

    return 0;
}
int video_decode_thread (void *argv){
    static AVFrame *frame = av_frame_alloc();

    void** p_argv=(void**)argv;

    int* quit=(int*)p_argv[0];
    VideoStream * is=(VideoStream *)p_argv[1];
    PacketQueue* video_queue=is->video_queue;
    printf("=>视频解码线程\n");

    AVPacket pkt;
    SDL_Event e;
    while (!*quit){
        int ret=video_queue->dequeue(&pkt,0);

        if (!ret){
            continue;
        }
        ret = avcodec_send_packet(is->video_decoder, &pkt);
        if(ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
            av_packet_unref(&pkt);
            break;
        }
        av_packet_unref(&pkt);

        bool exitLoop= false;

        while( ret >= 0){
            ret = avcodec_receive_frame(is->video_decoder, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            } else if( ret < 0) {
                exitLoop= true;
                break;
            }
            //这里控制帧率

            int pts=frame->pts;
//            av_frame_get_best_effort_timestamp(frame);
            AVFrame *new_frame=av_frame_alloc();
            av_frame_move_ref(new_frame,frame);
            is->frame_queue->enqueue(new_frame);
            //开始播放了，这几计算需要的延迟
            AVRational base_time={1,AV_TIME_BASE};

            int64_t frame_pts=av_rescale_q_rnd(pts,is->stream_time_base,base_time,static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));



            // 以系统时间戳system_pts为基准，如果packet的时间戳 晚于system_pts，等待 一定时间后再发生
            long long current_timestamp_ms = get_time_us();
            long long system_pts=current_timestamp_ms-is->start_time_ms;
            if (frame_pts>system_pts){
                av_usleep((frame_pts-system_pts));
            }

            e.type=VIDEO_NEW_FRAME;
            SDL_PushEvent(&e);
//            av_frame_unref(frame);
        }
        if (exitLoop){
            break;
        }
    }

    e.type=THREAD_QUIT;
    SDL_PushEvent(&e);

    return 0;
}
void read_audio_callback(void *userdata, Uint8 * stream,int len){
    static Uint8 AUDIO_DATA[BLOCK_SIZE];
    int AUDIO_SIZE=0; //AUDIO_DATA中有效的音频数据
    int AUDIO_INDEX=0;//下次拷贝时候从AUDIO_DATA的索引

    VideoStream *is=(VideoStream *)userdata;

    memset(stream,0,len);
    if (!is->audio_decoder){
        return;
    }


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


int main(int  argc,char *argv[]){
    if (argc<2){
        fprintf( stdout, "Usage thread_player videoname\n");
        return -1;
    }
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    VideoStream is;
    memset(&is,0,sizeof(is));
    //////////////////////////窗口的初始化//////////////////////////////////////
    SDL_Window *win = SDL_CreateWindow("线程播放器",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       0, 0,
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, 0);
    SDL_Texture *  texture=NULL;
    //////////////////////////音频的初始化//////////////////////////////////////
    SDL_AudioSpec param;
    //音频三要素,设卡的默认输出
    param.channels=Player_AUDIO_CHANNEL;
    param.format=AUDIO_S16SYS;//有符号的16位
    param.freq=Player_AUDIO_SAMPLE_RATE;
    param.callback=read_audio_callback;
    param.userdata=&is;//透传参数
    param.silence=0; //表示0 代表没有声音
    param.samples=1024;//我理解这个 是声卡采集的buf大小

    if(SDL_OpenAudio(&param,NULL)){
        SDL_Log("无法打开Audio设备\n");
        return  -1;
    }
    //play audio
    SDL_PauseAudio(0);
    //////////////////////////子线程创建//////////////////////////////////////
    int quit=0;

    PacketQueue audio_packetQueue; //解复用的音频包
    PacketQueue video_packetQueue; //解复用的视频包
    FrameQueue  frame_queue;       //解码后的数据帧

    is.audio_queue=&audio_packetQueue;
    is.video_queue=&video_packetQueue;
    is.frame_queue=&frame_queue;
    is.infile=argv[1];




    //解复用的线程，需要输入文件名
    //解复用之后把数据包放入音频和视频队列中
    void* remux_thread_args[]={&quit,&is};
    SDL_CreateThread(remux_thread,"remux",remux_thread_args);


    void* video_decode_thread_args[]={&quit,&is};
    SDL_CreateThread(video_decode_thread,"video",video_decode_thread_args);

    int wg=0;
    //////////////////////////主循环//////////////////////////////////////

    while (1){
        SDL_Event event;
        SDL_WaitEvent(&event);
        AVFrame * frame=NULL;


//        int ii,jj,cnt;
        switch(event.type) {
            case SDL_QUIT: //需要通知子线程退出
                quit=1;
                break;
            case THREAD_QUIT: //子线程退出通知主线程
                wg+=1;
                if (wg==2){
                    goto ENDL;
                }
                break;
            case VIDEO_LOAD_EVENT:
                //窗口宽度默认1280
                SDL_SetWindowSize(win, 1280,is.video_decoder->height*1280/is.video_decoder->width);
                texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_IYUV,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            is.video_decoder->width,
                                            is.video_decoder->height);
                break;
            case VIDEO_NEW_FRAME:
                if (!texture){
                    break;
                }

                while (frame=is.frame_queue->dequeue(0)){

                SDL_UpdateYUVTexture(texture,
                                     NULL,
                                     frame->data[0], frame->linesize[0],
                                     frame->data[1], frame->linesize[1],
                                     frame->data[2], frame->linesize[2]);
                }


                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);

//                 cnt=2;
//                for ( ii = 0; ii < cnt; ++ii) {
//                    for ( jj = 0; jj < cnt; ++jj) {
//                        SDL_Rect screen_rect;
//                        screen_rect.w=1280/cnt;
//                        screen_rect.h=is.video_decoder->height*1280/is.video_decoder->width/cnt;
//                        screen_rect.x= 1280/cnt *ii;
//                        screen_rect.y=is.video_decoder->height*1280/is.video_decoder->width/cnt*jj;
//                        SDL_RenderCopy(renderer,texture,NULL,&screen_rect);
//                    }
//                }
//                SDL_RenderPresent(renderer);
                av_frame_free(&frame);
                break;
            default:
                break;
        }
    }

    ENDL:
    if(renderer){
        SDL_DestroyRenderer(renderer);
    }
    if (texture){
        SDL_DestroyTexture(texture);
    }
    SDL_CloseAudio();
    SDL_Quit();


}