#include "SDL.h"

#define WIDTH 800
#define HEIGHT 600
#define FPS  12
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define THREAD_QUIT  (SDL_USEREVENT + 2)

int  timeTickerFunc(void *data){
    int* isWorking=data;
    SDL_Event e;
    while (*isWorking){
        e.type=REFRESH_EVENT;
        SDL_Delay(1000/FPS);
        SDL_PushEvent(&e);
    }
    e.type=THREAD_QUIT;
    SDL_PushEvent(&e);
}
#define BLOCK_SIZE 4096000*4
char buf[BLOCK_SIZE];

//SDL_CreateThread
//SDL_UpdateTexture
int main(int  argc,char *argv[]){
    if (argc<2){
        return -1;
    }

    FILE *video_fd = NULL;
    const int video_width =  2940, video_height = 1912;
    int w_width = WIDTH, w_height = HEIGHT;

    const unsigned int yuv_frame_len = video_width * video_height * 12 / 8;
    video_fd = fopen(argv[1], "rb");
    if( !video_fd ){
        fprintf(stderr, "Failed to open yuv file\n");
        return -1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window=SDL_CreateWindow("hello World",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         w_width,w_height,
                                         SDL_WINDOW_RESIZABLE);
    if (window==NULL){
        SDL_Log("Could not create window: %s\n",SDL_GetError());
        return -1;
    }

    SDL_Renderer * render=SDL_CreateRenderer(window,-1,0);
    //创建YUV的纹理,Stream类型,纹理的W,H一定是 视频的分辨率
    SDL_Texture* texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,video_width,video_height);

    int isWorking=1;
    SDL_CreateThread(timeTickerFunc,"控制帧率的线程",&isWorking);

    int quit=1;
    do {
        SDL_Event  event;
        SDL_WaitEvent(&event);
//        SDL_PollEvent(&event);
        switch (event.type) {
            case THREAD_QUIT:
                quit=0;
                break;
            case SDL_QUIT:
                isWorking=0;
                break;
            case SDL_WINDOWEVENT:
                SDL_GetWindowSize(window, &w_width, &w_height);
                break;
        }

        if (event.type==REFRESH_EVENT){

            int n=fread(buf,1,yuv_frame_len,video_fd);
            if (n!=yuv_frame_len){
                return -1;
            }

            //把内存数据buf更新的纹理中
            if(SDL_UpdateTexture(texture,NULL,buf,video_width)){
                exit(1);
            }

            //这里的清屏很重要，不然的画 画面的四周围都会花屏
            SDL_SetRenderDrawColor(render,0,0,0,255);
            SDL_RenderClear(render);

            //把纹理 渲染到rect指定的区域。 video_width->widow_width, video_height->window_heiht
            //设置绘制到屏幕的区域


            //视频填充窗口展示
//            SDL_Rect screen_rect;
//            screen_rect.x= screen_rect.y=0;
//            screen_rect.w=w_width;screen_rect.h=w_height;
//            SDL_RenderCopy(render,texture,NULL,&screen_rect);
            //四宫格展示
            int cnt=2;
            for (int ii = 0; ii < cnt; ++ii) {
                for (int jj = 0; jj < cnt; ++jj) {
                    SDL_Rect screen_rect;
                    screen_rect.w=w_width/cnt;
                    screen_rect.h=w_height/cnt;
                    screen_rect.x= w_width/cnt *ii;
                    screen_rect.y=w_height/cnt*jj;
                    SDL_RenderCopy(render,texture,NULL,&screen_rect);
                }
            }




            SDL_RenderPresent(render);
        }





    } while (quit);


    if (video_fd){
        fclose(video_fd);
    }
    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}