#include "audioplayer.h"

audioPlayer::audioPlayer(QObject *parent)
    : QThread{parent}
{
    this -> userdata = new userData;
    this -> userdata -> curr_len = 0;

    this -> isRunning = false;
    this -> isPaused = false;
    //初始化SDL2库并加载所需的子系统
    int ret = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    if(ret  < 0){
        SDL_Log("sdl initialize is failed %s",SDL_GetError());
        return;
    }
}
audioPlayer::~audioPlayer(){
    SDL_PauseAudioDevice(this -> device_id,1);
    SDL_CloseAudioDevice(this -> device_id);
    delete this -> userdata;
    SDL_Quit();
}

void audioCallback(void *userdata, Uint8 * stream,int len){

    userData * data = (userData*)userdata;
    if(data -> audio_len == 0)return;

    data -> curr_len += len;
    if(data -> audio_len < (Uint32)len){
        len = data -> audio_len;
        SDL_memcpy(stream,data -> audio_buf + data -> curr_len,len);
    }else{
        SDL_memcpy(stream,data -> audio_buf + data -> curr_len,len);
    }
    data -> audio_len -= len;
}

void audioPlayer::run() {
    this -> isRunning = true;
    SDL_AudioSpec  audio_spec;

    //加载音频文件
    const char * c_fileName = this -> fileName.toUtf8().constData();
    SDL_RWops * rw = SDL_RWFromFile(c_fileName,"rb");
    if(rw == NULL){
        qDebug()<<"文件打开失败";
        return;
    }
    int freesrc = 0;//不等于0表示总是释放数据源
    SDL_AudioSpec * re_spec = SDL_LoadWAV_RW(rw,freesrc,&audio_spec,&userdata -> audio_buf,&userdata -> audio_len);
    if(re_spec == NULL){
        SDL_Log("sdl load wav is failed %s",SDL_GetError());
        return;
    }

    //定义相关参数
    audio_spec.callback = audioCallback;
    audio_spec.channels = 2;
    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_S16LSB;
    audio_spec.samples = 4096; //以采样帧为单位的音频缓冲区大小(总采样数除以通道数)
    audio_spec.userdata = this -> userdata;

    //计算总的时长
    qint64 Mduration = (this -> userdata -> audio_len) / (audio_spec.freq * audio_spec.channels * 2);
    emit sendMduration(Mduration);

    //打开音频设备（注意是打开，并不是播放）
    this -> device_id = SDL_OpenAudioDevice(NULL,0,&audio_spec,NULL,0);
    if(this -> device_id == 0){
        SDL_Log("sdl open device is failed %s",SDL_GetError());
        return;
    }

    SDL_QueueAudio(this -> device_id,this -> userdata -> audio_buf,this -> userdata -> audio_len);
    //0-表示播放；1-表示暂停
    SDL_PauseAudioDevice(this -> device_id,0);

    SDL_RWclose(rw); //关闭并清理SDL_RWops流。它释放任何流使用的资源，并释放SDL_RWops本身
}
