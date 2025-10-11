// chessaicontroller.h
#ifndef CHESSAICONTROLLER_H
#define CHESSAICONTROLLER_H

#include "chessboard.h"
#include <QObject>
#include <QPoint>

class ChessAIController : public QObject
{
    Q_OBJECT
public:
    explicit ChessAIController(ChessBoard* board, QObject *parent = nullptr);

    void setDifficulty(int difficulty); // 1-5, 5为最难

    // AI思考并返回最佳移动
    QPair<QPoint, QPoint> findBestMove(ChessPieceColor aiColor);

signals:
    void aiMoveReady(int fromX, int fromY, int toX, int toY);

public slots:
    void startThinking();

private:
    // 评估函数
    int evaluateBoard(ChessBoard& currentBoard) const;

    ChessPieceColor getOpponentColor(ChessPieceColor color) const;

    // 极小极大算法
    int minimax(int depth, int alpha, int beta, bool maximizingPlayer,ChessBoard& currentBoard);

    // 生成所有可能的移动
    QVector<QPair<QPoint, QPoint>> generateAllMoves(ChessPieceColor color, ChessBoard& currentBoard) const;

    ChessBoard* m_board;
    int m_difficulty;
    ChessPieceColor m_aiColor;
};

#endif // CHESSAICONTROLLER_H
