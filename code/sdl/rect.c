//
// Created by zzzlllll on 2021/12/11.
//
#include <SDL.h>
#include <libavutil/log.h>
#define DRAW_EVENT 9999
int TimerFunction(void* param){
    SDL_Event event;
    event.type=DRAW_EVENT;
    while (1){
        SDL_Delay(40);
        SDL_PushEvent(&event);
    }
}

void quickRender(SDL_Renderer* render,SDL_Texture *texture,SDL_Rect *rect);



int main(int  argc,char *argv[]){
    int w=800,h=600;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window=SDL_CreateWindow("hello World",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                          w,h,
                                         SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
    if (window==NULL){
        av_log(NULL,AV_LOG_ERROR,"Could not create window: %s\n",SDL_GetError());
        goto exit;
    }

    SDL_Renderer * render=SDL_CreateRenderer(window,-1,0);
    if(render==NULL){
        av_log(NULL,AV_LOG_ERROR,"Could not create render: %s\n",SDL_GetError());
        goto exit;
    }

    SDL_Texture* texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,w,h);
    if(!texture){
        av_log(NULL,AV_LOG_ERROR,"Could not create texture: %s\n",SDL_GetError());
        goto exit;
    }
    SDL_Thread * thread=SDL_CreateThread(TimerFunction,"TimerFunction",NULL);
    if(!thread){
        av_log(NULL,AV_LOG_ERROR,"Could not create thread: %s\n",SDL_GetError());
        goto exit;
    }

    SDL_Event  event;
    int quit=0;
    SDL_Rect rect;
    rect.x= w / 2;
    rect.y= h / 2;
    rect.w=30;
    rect.h=30;
    while (!quit){
        SDL_WaitEvent(&event);
//        SDL_WaitEventTimeout(&event,100);
//        SDL_PollEvent(&event);

//        av_log(NULL,AV_LOG_INFO,"type=%d\n",event.type);
        switch (event.type) {
            case SDL_QUIT:
                quit=1;
                break;
            case DRAW_EVENT:
                rect.x=rand()%w;
                rect.y=rand()%h;
                quickRender(render,texture,&rect);
                break;
        }

    }
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    exit:
    SDL_Quit();
    return 0;
}

void quickRender(SDL_Renderer* render,SDL_Texture *texture,SDL_Rect *rect) {
    SDL_SetRenderTarget(render,texture);//切换到texture
    //填充背景
    SDL_SetRenderDrawColor(render,0,255,0,255);//设置颜色
    SDL_RenderClear(render);

    SDL_RenderDrawRect(render,rect);
    SDL_SetRenderDrawColor(render,0,0,100,255);
    SDL_RenderFillRect(render,rect);


    SDL_SetRenderTarget(render,NULL);//切换会屏幕
    SDL_RenderCopy(render,texture,NULL,NULL);//交换


    SDL_RenderPresent(render);//渲染
}