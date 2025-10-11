#ifndef AIGAMECONTROLLER_H
#define AIGAMECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QPoint>
#include <iostream>
#include "board.h"
#include "player.h"
#include "common.h"

class Client; // 前向声明

class AIGameController : public QObject
{
    Q_OBJECT

public:
    explicit AIGameController(Client* client, QObject* parent = nullptr);
    ~AIGameController();

    // 初始化游戏
    void initializeGame();

    // 开始游戏
    void startGame();

    // 退出游戏
    void exitGame();

    // 获取游戏状态
    GameState getGameState() const { return m_gameState; }

    PieceType getAIPieceType() const { return m_aiPlayer ? m_aiPlayer->getPieceType() : PieceType::Empty; }
    PieceType getCurrentPiece() const { return m_currentPlayer; }

    bool getIsAITurn(){return this ->m_isAITurn;}
    void humanMadeMove();

    // 和客户端同步棋盘的方法
    void syncBoardFromClient();
    void syncBoardToClient();
    // 同步当前玩家信息
    void syncPlayerFromClient();
    void syncPlayerToClient();

signals:
    // 游戏状态变化信号
    void gameStateChanged(GameState newState);
    // AI下棋信号
    void aiMadeMove(int x, int y);
    // 退出到选择页面信号
    void exitToSelection();

public slots:
    // 处理计时器更新
    void updateGameTimer();
    // AI思考并下棋
    void aiMakeMove();

private:
    // AI评估函数 - 评估当前位置的分数
    int evaluatePosition(int x, int y, PieceType piece) const;
    // AI决策函数 - 选择最佳落子位置
    QPoint findBestMove() const;
    // 检查是否能够获胜
    bool checkImmediateWin(PieceType piece) const;
    // 检查是否需要防守
    bool checkNeedDefense(PieceType piece) const;

private:
    Client* m_client;          // 主客户端引用
    Board* m_board;            // 棋盘
    Player* m_humanPlayer;     // 人类玩家
    Player* m_aiPlayer;        // AI玩家
    QTimer* m_gameTimer;       // 游戏计时器
    GameState m_gameState;     // 游戏状态
    int m_elapsedTime;         // 已用时间（秒）
    bool m_isAITurn;           // 是否是AI的回合
    PieceType m_currentPlayer; // 当前玩家
};

#endif // AIGAMECONTROLLER_H
