#pragma once
#include <QString>
#include <iostream>
#include "board.h" // 包含PieceType枚举



class Player {
public:
    // 构造函数
    explicit Player(const QString& name = "",
                    PieceType pieceType = PieceType::Empty,
                    bool isAI = false);

    // 获取玩家名称
    QString getName() const;
    // 获取玩家棋子类型
    PieceType getPieceType() const;
    // 获取玩家当前分数
    int getScore() const;
    // 检查是否是AI玩家
    bool isAI() const;
    // 设置AI状态
    void setAI(bool ai);
    // 增加分数
    void addScore(int points);
    // 重置分数
    void resetScore();
    // 设置棋子类型
    void setPieceType(PieceType piece);
    // 设置玩家名称
    void setName(const QString& name);

    // 获取胜率（需要总游戏场次）
    double getWinRate(int totalGames) const;
    // 获取当前连胜/连败场次
    int getStreak() const;
    // 获取等级（基于分数）
    int getLevel() const;

    // 总的胜利次数
    int getTotalWins() const;
    int getTotalLosses() const;
    void addWin();
    void addLoss();
    void resetStats();

    // 处理平局
    int getTotalDraws() const;
    void addDraw();

    // 记录连胜的情况
    void resetStreak();
    void updateStreak(bool won); // 新增：根据胜负更新连胜

    // 在 player.cpp 中实现
    void setScore(int score) { m_score = score; }
    void setWins(int wins) { m_totalWins = wins; }
    void setLosses(int losses) { m_totalLosses = losses; }
    void setDraws(int draws) { m_totalDraws = draws; }
    void setStreak(int streak) { m_currentStreak = streak; }

    void copyFrom(const Player&player);
    Player(const Player&player);
    Player&operator=(const Player&player);

private:
    QString m_name;       // 玩家名称
    PieceType m_pieceType; // 使用的棋子类型（黑/白）
    int m_score;          // 当前分数
    bool m_isAI;          // 是否为AI玩家

    int m_streak;          // 连胜/连败场次
    int m_totalWins;       // 总胜利场次
    int m_totalLosses;     // 总失败场次
    int m_currentStreak;   // 当前连胜场次（正数为连胜，负数为连败）
    int m_totalDraws; // 记录平局
};
