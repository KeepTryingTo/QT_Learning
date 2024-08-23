// #include "sdlaudioplayer.h"

#include <QApplication>

#include <SDL2/SDL.h>

#undef main

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;
    SDL_Event e;
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    bool running = true;
    SDL_Rect rect{50,50,150,150};

    SDL_CreateWindowAndRenderer(800,600,0,&window,&renderer);
    QString title = QString("This is a demo");
    SDL_SetWindowTitle(window,title.toStdString().c_str());

    while(running){
        while(SDL_PollEvent(&e)){ //用于从事件队列中获取待处理的事件
            if(e.type == SDL_QUIT){
                running = false;
            }
            if(e.type == SDL_KEYDOWN){ //按下键盘响应事件
                switch(e.key.keysym.sym){
                case SDLK_LEFT:
                    rect.x -= 5;
                    break;
                case SDLK_RIGHT:
                    rect.x += 5;
                    break;
                case SDLK_ESCAPE://按下ESC键，退出层序
                    running = false;
                    break;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer,0,255,255,255);//首先将整个窗口渲染为青色
        SDL_RenderClear(renderer);//清除渲染器缓冲区的内容

        SDL_SetRenderDrawColor(renderer,255,255,255,255);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&rect); //渲染矩形，并且矩形填充为白色

        SDL_RenderPresent(renderer); //显示渲染器缓冲区的内容
        SDL_Delay(10);
    }
    SDL_Quit();
    return a.exec();
}
