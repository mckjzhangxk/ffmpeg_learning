#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <SDL.h>


typedef struct _VideoState {
    AVCodecContext *video_avctx;  //解码上下文
    AVPacket       *pkt;
    AVFrame        *vframe;
    SDL_Texture    *texture;
}VideoState;

SDL_Renderer * renderer=NULL;

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
    char buf[1024];

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
        av_log(NULL, AV_LOG_INFO, "usage simple_player  a.mp4\n");
        exit(-1);
    }

    const char *src = argv[1];

    //2. 初始化SDL，并创建窗口和Render
    //2.1
    if(SDL_Init(SDL_INIT_VIDEO)) {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    //2.2 creat window from SDL
    int w_width=640,w_height=480;
    SDL_Window *win = SDL_CreateWindow("Simple Player",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           w_width, w_height,
                           SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(win, -1, 0);



    //3. 打开多媒体文件，并获得流信息
    int ret=0;
    AVFormatContext *fmtCtx=NULL;
    if((ret = avformat_open_input(&fmtCtx, src, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto __END;
    }

    if((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto __END;
    }

    //4. 查找最好的视频流
    int idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(idx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include audio stream!\n");
        goto __END;
    }
    //5. 根据流中的codec_id, 获得解码器
    AVStream *inStream = fmtCtx->streams[idx];
    const AVCodec *dec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if(!dec){
        av_log(NULL, AV_LOG_ERROR, "Could not find libx264 Codec");
        goto __END;
    }
    //6. 创建解码器上下文

    AVCodecContext * ctx = avcodec_alloc_context3(dec);
    if(!ctx){
        av_log(NULL, AV_LOG_ERROR, "NO MEMRORY\n");
        goto __END;
    }
    //7. 从视频流中拷贝解码器参数到解码器上文中
    ret = avcodec_parameters_to_context(ctx, inStream->codecpar);
    if(ret < 0){
        av_log(ctx, AV_LOG_ERROR, "Could not copyt codecpar to codec ctx!\n");
        goto __END;
    }
    //8. 绑定解码器上下文
    ret = avcodec_open2(ctx, dec , NULL);
    if(ret < 0) {
        av_log(ctx, AV_LOG_ERROR, "Don't open codec: %s \n", av_err2str(ret));
        goto __END;
    }
    //9. 根据视频的宽/高创建纹理
    int video_width = ctx->width;
    int video_height = ctx->height;
    SDL_PixelFormatEnum pixformat = SDL_PIXELFORMAT_IYUV;
    SDL_Texture *  texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                video_width,
                                video_height);



    AVPacket  *pkt=av_packet_alloc();
    AVFrame   *frame=av_frame_alloc();

    //分配，并且都置0
    VideoState *is = av_mallocz(sizeof(VideoState));
    is->vframe=frame;
    is->texture=texture;
    is->pkt=pkt;
    is->video_avctx=ctx;



    //10. 从多媒体文件中读取数据，进行解码
    while(av_read_frame(fmtCtx, pkt) >= 0) {
        if(pkt->stream_index == idx) {
            //11. 对解码后的视频帧进行渲染
            video_decode(is);
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
    if(frame){
        av_frame_free(&frame);
    }

    if(pkt){
        av_packet_free(&pkt);
    }

    if(ctx){
        avcodec_free_context(&ctx);
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
        av_freep(is);
    }
    SDL_Quit();
    return 0;
}