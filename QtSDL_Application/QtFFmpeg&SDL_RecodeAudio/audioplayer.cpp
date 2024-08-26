#include "audioplayer.h"

#include <QDebug>
#include <QFileInfo>

audioPlayer::audioPlayer(QObject *parent)
    : QThread{parent}
{
    this -> userdata = new userData;
    this -> userdata -> curr_len = 0;

    this -> isRunning = false;
    this -> isPaused = false;

    SDL_version x;
    SDL_VERSION(&x);
    qDebug()<<"sdl version: "<<x.major<<" "<<x.minor<<" "<<x.patch;
    //初始化SDL2库并加载所需的子系统
    int ret = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    if(ret  < 0){
        SDL_Log("sdl initialize is failed %s",SDL_GetError());
        return;
    }

    this -> audio_spec = new SDL_AudioSpec;
}
audioPlayer::~audioPlayer(){
    SDL_PauseAudioDevice(this -> device_id,1);
    SDL_CloseAudioDevice(this -> device_id);
    delete this -> userdata;
    SDL_Quit();
}

void audioCallback(void *userdata, Uint8 * stream,int len){

    SDL_memset(stream,0,len);

    userData * data = (userData*)userdata;
    if(data -> audio_len == 0)return;

    if(data -> audio_len < (Uint32)len){
        len = data -> audio_len;
        SDL_memcpy(stream,data -> audio_buf + data -> curr_len,len);
        // SDL_MixAudio(stream,data -> audio_buf + data -> curr_len,len,SDL_MIX_MAXVOLUME);
    }else{
        SDL_memcpy(stream,data -> audio_buf + data -> curr_len,len);
        // SDL_MixAudio(stream,data -> audio_buf + data -> curr_len,len,SDL_MIX_MAXVOLUME);
    }
    data -> curr_len += len;
    data -> audio_len -= len;
}

// void audioPlayer::run() {
//     this -> isRunning = true;
//     SDL_AudioSpec  audio_spec;
//     SDL_AudioSpec  obtain_spec;

//     //定义相关参数
//     audio_spec.callback = audioCallback;
//     audio_spec.channels = 2;
//     audio_spec.freq = 44100;
//     audio_spec.format = AUDIO_S16LSB;
//     audio_spec.samples = 4096; //以采样帧为单位的音频缓冲区大小(总采样数除以通道数)
//     audio_spec.userdata = this -> userdata;

//     //加载音频文件
//     QFile file(this -> fileName);
//     if(!file.open(QIODevice::ReadOnly)){
//         qDebug()<<"打开文件失败";
//         SDL_Quit();
//         return;
//     }
//     QFileInfo info(this -> fileName);

//     //计算总的时长
//     qint64 Mduration = (info.size()) / (audio_spec.freq * audio_spec.channels * 2);
//     emit sendMduration(Mduration);

//     //打开音频设备（注意是打开，并不是播放）
//     this -> device_id = SDL_OpenAudio(&audio_spec,&obtain_spec);
//     if(this -> device_id < 0){
//         SDL_Log("sdl open device is failed and %s",SDL_GetError());
//         SDL_Quit();
//         return;
//     }
//     //0-表示播放；1-表示暂停
//     SDL_PauseAudio(0);

//     char data[4096 * 4] = {0};
//     while(!file.atEnd()){
//         this -> userdata -> audio_len = file.read(data,sizeof(data));
//         this -> userdata -> curr_len = 0;
//         if(this -> userdata -> audio_len > 0){
//             this -> userdata -> audio_buf = (Uint8*)data;
//             //不断的播放过程，直到当前读取的音频数据播放完毕，继续下一段音频的播放
//             while(this -> userdata -> audio_len > 0){
//                 SDL_Delay(1);
//             }
//         }else{
//             break;
//         }
//     }

//     emit sendFinished();
//     SDL_Quit();
// }

void audioPlayer::getSpec(){
    //加载音频文件
    qDebug()<<"filename = "<<this -> fileName;
    const char * c_fileName = this -> fileName.toUtf8().constData();
    rw = SDL_RWFromFile(c_fileName,"rb");
    if(rw == NULL){
        qDebug()<<"文件打开失败";
        return;
    }
    int freesrc = 0;//不等于0表示总是释放数据源
    SDL_AudioSpec * re_spec = SDL_LoadWAV_RW(rw,freesrc,audio_spec,&userdata -> audio_buf,&userdata -> audio_len);
    if(re_spec == NULL){
        SDL_Log("sdl load wav is failed and %s",SDL_GetError());
        return;
    }

    //定义相关参数
    audio_spec -> callback = audioCallback;
    audio_spec -> channels = 2;
    audio_spec -> freq = 44100;
    audio_spec -> format = AUDIO_S16LSB;
    audio_spec -> samples = 4096; //以采样帧为单位的音频缓冲区大小(总采样数除以通道数)
    audio_spec -> userdata = this -> userdata;

    //计算总的时长
    this -> Mduration = (this -> userdata -> audio_len) / (audio_spec -> freq * audio_spec -> channels * 2);
    emit sendMduration(this -> Mduration);
}

void audioPlayer::run() {
    //打开音频设备（注意是打开，并不是播放）
    this -> device_id = SDL_OpenAudioDevice(NULL,0,audio_spec,NULL,0);
    if(this -> device_id == 0){
        SDL_Log("sdl open device is failed and %s",SDL_GetError());
        return;
    }

    //0-表示播放；1-表示暂停
    SDL_PauseAudioDevice(this -> device_id,0);

    SDL_RWclose(rw); //关闭并清理SDL_RWops流。它释放任何流使用的资源，并释放SDL_RWops本身
}

