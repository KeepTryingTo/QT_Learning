#pragma once
#include <QObject>
#include <QPoint>
#include <QMutex>
#include <QMutexLocker>
#include "piecetype.h"


class Board : public QObject {
    Q_OBJECT
public:
    explicit Board(int size = 15, QObject *parent = nullptr);

    mutable QMutex m_mutex; // 添加互斥锁

    // 添加拷贝构造函数和赋值运算符
    Board(const Board& other);
    Board& operator=(const Board& other);

    // 基础操作
    bool placePiece(int x, int y, PieceType piece);
    PieceType getPiece(int x, int y) const;
    void clear();

    // 添加复制功能
    void copyFrom(const Board& other);
    QVector<QVector<PieceType>> getGridState() const;
    void setGridState(const QVector<QVector<PieceType>>& grid);

    // 游戏状态
    bool checkWin(int x, int y, PieceType piece) const;
    // 棋盘是否满
    bool isFull() const;
    int getSize() const { return m_size; }

    QVector<QPoint> getWinningLine() const;

    QVector<QVector<PieceType>> getBoard(){
        return this -> m_grid;
    }
    void setPos(int x, int y, PieceType piece){
        this -> m_grid[x][y] = piece;
    }

signals:
    // 通知棋子落下
    void piecePlaced(int x, int y, PieceType piece);
    void boardCleared();

private:
    int m_size;
    // 棋盘上的棋子情况
    QVector<QVector<PieceType>> m_grid;

    bool checkDirection(int x, int y, int dx, int dy, PieceType piece) const;
    QVector<QPoint> getWinningLineInDirection(int x, int y, int dx, int dy, PieceType piece) const;
};
