// chessboard.h
#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "piece.h"
#include <QObject>
#include <QVector>

class ChessBoard : public QObject
{
    Q_OBJECT
public:
    explicit ChessBoard(QObject *parent = nullptr);

    ChessBoard(const ChessBoard& other);
    ChessBoard& operator=(const ChessBoard& other);

    // 初始化棋盘
    void initialize();

    // 获取棋子
    ChessPiece getPiece(int x, int y) const;

    // 移动棋子
    bool movePiece(int fromX, int fromY, int toX, int toY);

    // 检查移动是否合法
    bool isValidMove(int fromX, int fromY, int toX, int toY) const;

    // 检查游戏状态
    bool isCheckmate(ChessPieceColor color) const;
    bool isCheck(ChessPieceColor color) const;
    bool isStalemate() const;

    // 获取所有合法移动
    QVector<QPoint> getValidMoves(int x, int y) const;

    // 棋盘状态
    QVector<QVector<ChessPiece>> getBoardState() const;

signals:
    void pieceMoved(int fromX, int fromY, int toX, int toY);
    void boardInitialized();
    void gameOver(ChessPieceColor winner);

private:
    // 判断每一种棋子移动的合法性
    bool isValidGeneralMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidAdvisorMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidElephantMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidHorseMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidRookMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidCannonMove(const ChessPiece& piece, int toX, int toY) const;
    bool isValidPawnMove(const ChessPiece& piece, int toX, int toY) const;

    // 检查路径是否畅通
    bool isPathClear(int fromX, int fromY, int toX, int toY) const;
    bool isInPalace(int x, int y, ChessPieceColor color) const;
    bool isCrossRiver(int x, ChessPieceColor color) const; // 跨河

    QVector<QVector<ChessPiece>> m_board;
};

#endif // CHESSBOARD_H
