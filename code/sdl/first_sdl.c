
#include "SDL.h"
/*
 *当把没有执行SDL_WaitEvent(&event)的这一段代码的时候，只用SDL_Delay的时候，
 * sdl窗口一直不显示，和视频教程不同，不要灰心，这是正常现象！
 * */
//知识点1
//SDL所有事件都是放在一个队列中
//所有对时间的操作，都是针对队列的操作

//知识点2
//SDL事件类别
//  SDL_WindowEvent SDL_KeyboardEvent SDL_MouseMotionEvent


//知识点3
//对事件的处理2中方式
//  SDL_PollEvent:轮训并且等待事件
//  SDL_WaitEvent:方法阻塞，直到有event唤起阻塞，
int main(int  argc,char *argv[]){

    int w=800,h=600;
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    SDL_Window * window=SDL_CreateWindow("hello World",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                          w,h,
                                         SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);

    SDL_Renderer * render=SDL_CreateRenderer(window,-1,0);

    SDL_SetRenderDrawColor(render,255,0,0,255);
    SDL_RenderClear(render);

    //可以认为是发送给显示缓存
    SDL_RenderPresent(render);


    int quit=1;


    do {
        SDL_Event event;
        //没有消息的时候阻塞，直到有消息唤起这个方法
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                quit=0;
                break;
            default:
                SDL_Log("Get Event %d\n",event.type);
        }
    } while (quit);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}