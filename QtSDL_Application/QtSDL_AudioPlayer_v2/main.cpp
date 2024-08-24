// #include "sdlaudioplayer.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QDebug>
#include <QCoreApplication>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "audioplayer.h"

#undef main

struct Button {
    SDL_Rect rect;
    SDL_Color color;
    Button(int x,int y,int w,int h,Uint8 r,Uint8 g,Uint8 b,Uint8 a){
        this -> rect.x = x;
        this -> rect.y = y;
        this -> rect.w = w;
        this -> rect.h = h;
        this -> color.r = r;
        this -> color.g = g;
        this -> color.b = b;
        this -> color.a = a;
    }
};
//检测鼠标点击的范围是否再按钮的给定区域
int isMouseOverButton(Button * button,int mouseX,int mouseY){
    return (mouseX > button->rect.x && mouseX < button->rect.x + button->rect.w &&
            mouseY > button->rect.y && mouseY < button->rect.y + button->rect.h);
}

TTF_Font * initTTF(){
    int ret = TTF_Init();
    if(ret == -1){
        SDL_Log("TTF init is failed %s",TTF_GetError());
        return NULL;
    }
    TTF_Font * font = TTF_OpenFont("C:\\Windows\\Fonts\\times.ttf",24);
    return font;
}

// 绘制文本
SDL_Texture* loadTextTexture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        SDL_Log("Unable to create text surface! TTF_Error: %s\n", TTF_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(texture == NULL){
        SDL_Log("from SDL_Surface to SDL_Texture is failed %s",SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}

//获取文本高宽
SDL_Rect getTextureSize(SDL_Texture * textTexture,SDL_Rect textRect){
    SDL_Rect textDstRect;  // 文本目标矩形

    // 获取文本的宽高
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    textDstRect = { textRect.x + (textRect.w - textWidth) / 2,
                    textRect.y + (textRect.h - textHeight) / 2,
                    textWidth, textHeight };
    return textDstRect;
}
//打开文件路径
QString getFilePath(){
    QString wav_path = QFileDialog::getOpenFileName(
        nullptr,QString("打开文件"),".",
        "All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)");
    if(wav_path.isEmpty()){
        QMessageBox::information(nullptr,"提示","未选择文件");
        return "";
    }
    QFileInfo fileinformation(wav_path);
    return wav_path;
}

QString updateDuration(qint64 Mduration){
    QTime totalTime((Mduration / 3600) % 60,(Mduration / 60) % 60,Mduration % 60,(Mduration * 1000) % 1000);

    QString format = "mm:ss";
    if(Mduration > 3600){
        format = "hh:mm:ss";
    }
    return totalTime.toString(format);
}

//初始化图像子系统
void InitImage(){
    SDL_version x;
    SDL_IMAGE_VERSION(&x);
    qDebug()<<"SDL2_Image version: "<<x.major<<" "<<x.minor<<" "<<x.patch;

    int ret = IMG_Init(IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP | IMG_INIT_JPG);//初始化图像子系统
    if(!ret){
        SDL_Log("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_Quit();
    }
}
//打开图像
SDL_Texture * openImg(SDL_Renderer * renderer,const char * file){
    SDL_Texture * texture = IMG_LoadTexture(renderer,file);
    if(!texture){
        SDL_Log("load image is failed and %s",IMG_GetError());
        return NULL;
    }
    return texture;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;
    SDL_Event e;
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    InitImage();//初始化图像子系统

    SDL_version x;
    SDL_VERSION(&x);
    qDebug()<<"SDL2 version: "<<x.major<<" "<<x.minor<<" "<<x.patch;
    bool running = true;

    //定义打开文件按钮
    Button * open = new Button(640,20,130,50,192,192,192,255);

    //定义播放按钮
    Button * player = new Button(640,120,130,50,192,192,192,255);
    //定义暂停按钮
    Button * pause = new Button(640,220,130,50,192,192,192,255);
    //定义关闭窗口按钮
    Button * close = new Button(640,420,130,50,192,192,192,255);

    //创建一个用于显示内容的矩阵框
    SDL_Rect imgShow {20,20,600,450};

    //定义显示进度时间和总时间的框
    SDL_Rect label_time1{20,520,150,50};
    SDL_Rect label_time2{490,520,150,50};

    //创建和渲染窗口
    int ret = SDL_CreateWindowAndRenderer(800,600,0,&window,&renderer);
    if(ret == -1){
        SDL_Log("rend window is failed %s",SDL_GetError());
        delete open;delete player;delete pause;delete close;
        return 0;
    }
    QString title = QString("This is a audio player");
    SDL_SetWindowTitle(window,title.toStdString().c_str());

    //加载图像
    QString imgPath = QString("D:/SoftwareFamily/QT/projects/QtFFmpeg_Application/QtSDL_AudioPlayer_v3/resources/KTG.jpg");
    SDL_Texture * imgTexTure = openImg(renderer, imgPath.toStdString().c_str()); // 替换为你的图像文件路径
    if (!imgTexTure) {
        delete open;delete player;delete pause;delete close;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    //初始化和加载字体
    TTF_Font * font = initTTF();
    if(font == NULL){
        SDL_Log("TTF init is failed %s",TTF_GetError());
        delete open;delete player;delete pause;delete close;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    //初始化音频播放对象
    audioPlayer * audioplayer = new audioPlayer;

    //给"打开文件按钮"绘制文本
    SDL_Color textColor = {0,0,0,255};
    const char * openText ="Open";
    SDL_Texture * openTexture = loadTextTexture(renderer,font,openText,textColor);
    //获取当前文本高宽
    SDL_Rect openTextRect = getTextureSize(openTexture,open -> rect);

    //给"打开文件按钮"绘制文本
    const char * playerText ="Player";
    SDL_Texture * playerTexture = loadTextTexture(renderer,font,playerText,textColor);
    //获取当前文本高宽
    SDL_Rect playerTextRect = getTextureSize(playerTexture,player -> rect);

    //给"打开文件按钮"绘制文本
    const char * pauseText ="Pause";
    SDL_Texture * pauseTexture = loadTextTexture(renderer,font,pauseText,textColor);
    //获取当前文本高宽
    SDL_Rect pauseTextRect = getTextureSize(pauseTexture,pause -> rect);

    //给"打开文件按钮"绘制文本
    const char * closeText ="Close";
    SDL_Texture * closeTexture = loadTextTexture(renderer,font,closeText,textColor);
    //获取当前文本高宽
    SDL_Rect closeTextRect = getTextureSize(closeTexture,close -> rect);

    //记录打开的音频文件路径
    QString wavPath;

    qint64 Mduration;

    while(running){
        while(SDL_PollEvent(&e)){ //用于从事件队列中获取待处理的事件
            if(e.type == SDL_QUIT){
                running = false;
            }
            if(e.type == SDL_KEYDOWN){ //按下键盘响应事件
                switch(e.key.keysym.sym){
                case SDLK_ESCAPE://按下ESC键，退出层序
                    running = false;
                    break;
                }
            }else if(e.type == SDL_MOUSEBUTTONDOWN){
                int x,y;
                SDL_GetMouseState(&x,&y);
                //判断当前坐标和给定几个按钮，如果点击了相应的按钮，即响应当前按钮
                if(isMouseOverButton(open,x,y)){
                    wavPath = getFilePath();

                }else if(isMouseOverButton(player,x,y)){
                    audioplayer -> fileName = wavPath;
                    //打开WAV文件，并将总的时长绘制到指定的矩形框中
                    audioplayer -> getSpec();//加载WAV文件以及参数定义
                    Mduration = audioplayer -> Mduration;

                    audioplayer -> start();
                }else  if(isMouseOverButton(pause,x,y)){
                    //这个功能没有实现，暂时先留着
                }else if(isMouseOverButton(close,x,y)){
                    running = false;
                    SDL_DestroyTexture(imgTexTure);
                    TTF_CloseFont(font);
                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(window);
                    SDL_Quit();
                    audioplayer -> exit();
                    audioplayer -> wait();
                    audioplayer -> deleteLater();
                    delete open;delete player;delete pause;delete close;
                    QApplication::quit();
                }
            }
        }
        SDL_SetRenderDrawColor(renderer,0,0,0,255);//首先将整个窗口渲染为青色
        SDL_RenderClear(renderer);//清除渲染器缓冲区的内容

        int x,y;
        SDL_GetMouseState(&x,&y);
        //当鼠标移动到给定的按钮上面的时候就变为相应的颜色
        if(isMouseOverButton(open,x,y)){
            open -> color.r = 0;open -> color.g = 255;open -> color.b = 255;open -> color.a = 255;
        }else{
            open -> color.r = 192;open -> color.g = 192;open -> color.b = 192;open -> color.a = 255;
        }
        if(isMouseOverButton(player,x,y)){
            player -> color.r = 0;player -> color.g = 255;player -> color.b = 255;player -> color.a = 255;
        }else{
            player -> color.r = 192;player -> color.g = 192;player -> color.b = 192;player -> color.a = 255;
        }
        if(isMouseOverButton(pause,x,y)){
            pause -> color.r = 0;pause -> color.g = 255;pause -> color.b = 255;pause -> color.a = 255;
        }else{
            pause -> color.r = 192;pause -> color.g = 192;pause -> color.b = 192;pause -> color.a = 255;
        }
        if(isMouseOverButton(close,x,y)){
            close -> color.r = 0;close -> color.g = 255;close -> color.b = 255;close -> color.a = 255;
        }else{
            close -> color.r = 192;close -> color.g = 192;close -> color.b = 192;close -> color.a = 255;
        }

        //将所有的按钮绘制到窗口上以及矩形框
        SDL_SetRenderDrawColor(renderer,open -> color.r,open -> color.g,open -> color.b,open->color.a);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&open -> rect); //渲染矩形，并且矩形填充为白色

        SDL_SetRenderDrawColor(renderer,player -> color.r,player -> color.g,player -> color.b,player->color.a);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&player -> rect); //渲染矩形，并且矩形填充为白色

        SDL_SetRenderDrawColor(renderer,pause -> color.r,pause -> color.g,pause -> color.b,pause->color.a);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&pause -> rect); //渲染矩形，并且矩形填充为白色

        SDL_SetRenderDrawColor(renderer,close -> color.r,close -> color.g,close -> color.b,close->color.a);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&close -> rect); //渲染矩形，并且矩形填充为白色

        SDL_SetRenderDrawColor(renderer,192,192,192,255);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&label_time1); //渲染矩形，并且矩形填充为白色

        SDL_SetRenderDrawColor(renderer,192,192,192,255);//然后黑色窗口的基础上将指定区域渲染为白色
        SDL_RenderFillRect(renderer,&label_time2); //渲染矩形，并且矩形填充为白色

        //给所有的按钮绘制文字
        SDL_RenderCopy(renderer, openTexture, NULL, &openTextRect);
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerTextRect);
        SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseTextRect);
        SDL_RenderCopy(renderer, closeTexture, NULL, &closeTextRect);
        //显示图像
        SDL_RenderCopy(renderer,imgTexTure,NULL,&imgShow);

        //显示总时长时间
        QString timeString = updateDuration(Mduration); //从秒转换为时间格式hh::mm::ss
        const char * durationText = timeString.toLocal8Bit().data();
        SDL_Texture * durationTexture = loadTextTexture(renderer,font,durationText,textColor);
        //获取当前文本高宽
        SDL_Rect durationTextRect = getTextureSize(durationTexture,label_time2);
        SDL_RenderCopy(renderer, durationTexture, NULL, &durationTextRect);


        SDL_RenderPresent(renderer); //显示渲染器缓冲区的内容
        SDL_Delay(10);
    }
    delete open;delete player;delete pause;delete close;
    audioplayer -> exit();
    audioplayer -> wait();
    audioplayer -> deleteLater();
    SDL_DestroyTexture(imgTexTure);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return a.exec();
}
