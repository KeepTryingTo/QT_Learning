// common.h 或 gamestate.h
#ifndef COMMON_H
#define COMMON_H

enum GameState {
    NotStarted,    // 游戏未开始
    Playing,       // 游戏中
    Paused,        // 游戏暂停
    GameOver       // 游戏结束
};

#endif // COMMON_H
