// chessgamecontroller.h
#ifndef CHESSGAMECONTROLLER_H
#define CHESSGAMECONTROLLER_H

#include "chessboard.h"
#include "chesSAIController.h"
#include <QObject>

class ChessGameController : public QObject
{
    Q_OBJECT
public:
    explicit ChessGameController(QObject *parent = nullptr);

    void startGame(ChessPieceColor humanColor = ChessPieceColor::Red);
    void exitGame();

    // 人类玩家移动
    bool humanMove(int fromX, int fromY, int toX, int toY);

    ChessPieceColor getCurrentPlayer() const { return m_currentPlayer; }
    bool isGameOver() const { return m_gameOver; }

    ChessBoard * getBoard(){return this->m_board;}

signals:
    void gameStarted();
    void gameEnded(ChessPieceColor winner);
    void pieceMoved(int fromX, int fromY, int toX, int toY);
    void currentPlayerChanged(ChessPieceColor player);
    void errorOccurred(const QString& message);

private slots:
    void onAIMoveReady(int fromX, int fromY, int toX, int toY);

private:
    void switchPlayer();
    void checkGameState();

    ChessBoard* m_board;
    ChessAIController* m_aiController;
    ChessPieceColor m_currentPlayer;
    ChessPieceColor m_humanColor;
    bool m_gameOver;
};

#endif // CHESSGAMECONTROLLER_H
