#include "SDL.h"


#define WIDTH 800
#define HEIGHT 600

//纹理 Texture是对物体的描述信息，描述信息交给显卡计算并且渲染，是用户定义的.

//纹理渲染基本原理
//内存图片  ---(转换)---> 纹理 -----(交付)--->显卡--->(渲染命令)-->展示

//转换:经过渲染器转换，比如 Draw
//交付 :SDL_RenderCopy,交付显卡计算结果
//渲染命令:SDL_RenderPresent，展示命令
/////////////基本原理对应的API调用流程////////
//SDL_CreateTexture--->SDL_SetRenderTarget(render,texture)-->SDL_SetRenderDraw....-->SDL_RenderCopy-->SDL_RenderPresent
////////////////////////////////////////////////

////////////常用的命令总结////////////////////
//画图
//SDL_SetRenderDrawColor(render,0,0,0,255);
//SDL_RenderDrawRect(render,&targetRect);
//清理屏幕
//SDL_SetRenderDrawColor(render,0,0,255,255);
//SDL_RenderClear(render);
//填充
//SDL_SetRenderDrawColor(render,0,255,0,255);
//SDL_RenderFillRect(render,&targetRect);
////////////常用的命令总结结束/////////////////
int main(int  argc,char *argv[]){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window=SDL_CreateWindow("hello World",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH,HEIGHT,
                                         SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
    if (window==NULL){
        SDL_Log("Could not create window: %s\n",SDL_GetError());
        return -1;
    }

    SDL_Renderer * render=SDL_CreateRenderer(window,-1,0);


    //创建普通的纹理
    SDL_Texture* texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,WIDTH,HEIGHT);




    int quit=1;
    do {
        SDL_Event  event;
        //可以看出轮训与Wait的区别了吧？
        SDL_WaitEvent(&event);
//        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                quit=0;
                break;
        }
        SDL_Rect targetRect;
        targetRect.w=50;
        targetRect.h=50;
        targetRect.x=rand()%WIDTH;
        targetRect.y=rand()%HEIGHT;

        //设置纹理背景是红色
        SDL_SetRenderTarget(render,texture);
        SDL_SetRenderDrawColor(render,255,0,0,255);
        SDL_RenderClear(render);

        //画 矩形，这里是线段
        SDL_SetRenderDrawColor(render,0,0,0,255);
        SDL_RenderDrawRect(render,&targetRect);
        //设置矩形填充颜色是绿色
        SDL_SetRenderDrawColor(render,0,255,0,255);
        SDL_RenderFillRect(render,&targetRect);

        //设置窗口背景是蓝色
        SDL_SetRenderTarget(render,NULL);
        SDL_SetRenderDrawColor(render,0,0,255,255);
        SDL_RenderClear(render);

        //把纹理全部画到窗口上,由于是全部Texture覆盖窗口，所以看不到刚刚设置的蓝色背景
        SDL_RenderCopy(render,texture,NULL,NULL);
        SDL_RenderPresent(render);
    } while (quit);



    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}