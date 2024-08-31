#include <SDL2/SDL.h>

SDL_mutex* mutex;
SDL_cond* cond;

#undef main

int thread_func(void* data) {
    SDL_LockMutex(mutex);
    // 等待条件满足
    SDL_CondWait(cond, mutex);
    // 条件满足后执行相应操作
    SDL_Log("condition is satisfication and go on!");
    SDL_UnlockMutex(mutex);
    return 0;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO); // 初始化SDL（根据需要选择初始化项）

    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();

    SDL_Thread* thread = SDL_CreateThread(thread_func, "MyThread", NULL);

    // 延迟一段时间，模拟条件满足的情况
    SDL_Delay(6000);

    // 信号通知条件满足
    SDL_CondSignal(cond);

    // 等待线程结束
    int thread_result;
    SDL_WaitThread(thread, &thread_result);

    // 销毁互斥锁和条件变量
    SDL_DestroyMutex(mutex);
    SDL_DestroyCond(cond);

    // 退出SDL
    SDL_Quit();

    return 0;
}
