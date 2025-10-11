// chessgamecontroller.cpp
#include "chessgamecontroller.h"
#include <QDebug>
#include <QTimer>
#include <iostream>

ChessGameController::ChessGameController(QObject *parent)
    : QObject(parent)
    , m_board(new ChessBoard(this))
    , m_aiController(new ChessAIController(m_board, this))
    , m_currentPlayer(ChessPieceColor::Red)
    , m_humanColor(ChessPieceColor::Red)
    , m_gameOver(false)
{
    // 连接AI移动信号
    connect(m_aiController, &ChessAIController::aiMoveReady,this, &ChessGameController::onAIMoveReady);
}

void ChessGameController::startGame(ChessPieceColor humanColor)
{
    // 人类先手
    m_humanColor = humanColor;
    m_currentPlayer = ChessPieceColor::Red; // 红方先手
    m_gameOver = false;

    // 初始化棋盘
    m_board->initialize();

    // 打印初始棋盘状态
    qDebug() << "棋盘初始化完成，检查黑方棋子:";
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = m_board->getPiece(x, y);
            if (!piece.isEmpty() && piece.color == ChessPieceColor::Black) {
                qDebug() << "黑方棋子: 位置(" << x << "," << y << "), 类型:" << (int)piece.type;
            }
        }
    }

    // 设置AI难度
    m_aiController->setDifficulty(3); // 中等难度

    // 消息发送给main window
    emit gameStarted();
    emit currentPlayerChanged(m_currentPlayer);

    // 如果AI先手，开始思考
    if (m_currentPlayer != m_humanColor) {
        QTimer::singleShot(1000, this, [this]() {
            m_aiController->startThinking();
        });
    }
}

void ChessGameController::exitGame()
{
    m_gameOver = true;
    // 清理资源
    // 注意：m_board 和 m_aiController 是子对象，会自动删除
}

bool ChessGameController::humanMove(int fromX, int fromY, int toX, int toY)
{
    if (m_gameOver) {
        emit errorOccurred("游戏已结束，无法移动");
        return false;
    }

    if (m_currentPlayer != m_humanColor) {
        emit errorOccurred("不是您的回合");
        return false;
    }

    // 检查移动是否合法
    if (!m_board->isValidMove(fromX, fromY, toX, toY)) {
        emit errorOccurred("非法移动");
        return false;
    }

    if (m_board->movePiece(fromX, fromY, toX, toY)) {
        emit pieceMoved(fromX, fromY, toX, toY);

        // 检查游戏状态
        checkGameState();

        if (!m_gameOver) {
            // 切换玩家
            switchPlayer();

            // 如果是AI回合，启动AI思考 - 确保这里正确执行
            if (m_currentPlayer != m_humanColor) {
                qDebug() << "AI回合，开始思考...";
                QTimer::singleShot(500, this, [this]() { // 减少延迟以便调试
                    m_aiController->startThinking();
                });
            }
        }
        return true;
    }
    return false;
}

void ChessGameController::onAIMoveReady(int fromX, int fromY, int toX, int toY)
{
    qDebug() << "接收到AI移动: 从(" << fromX << "," << fromY
             << ") 到(" << toX << "," << toY << ")";

    if (m_gameOver) {
        qDebug() << "游戏已结束，忽略AI移动";
        return;
    }

    if (m_currentPlayer == m_humanColor) {
        qDebug() << "不是AI的回合，当前玩家是人类";
        return;
    }

    // 执行AI移动
    if (m_board->movePiece(fromX, fromY, toX, toY)) {
        qDebug() << "AI移动执行成功";
        emit pieceMoved(fromX, fromY, toX, toY);

        // 检查游戏状态
        checkGameState();

        if (!m_gameOver) {
            // 切换玩家
            switchPlayer();
            qDebug() << "切换玩家，当前玩家:"
                     << (m_currentPlayer == ChessPieceColor::Red ? "红方" : "黑方");
        }
    } else {
        qWarning() << "AI移动执行失败";
    }
}

void ChessGameController::switchPlayer()
{
    m_currentPlayer = (m_currentPlayer == ChessPieceColor::Red) ?
                          ChessPieceColor::Black : ChessPieceColor::Red;

    emit currentPlayerChanged(m_currentPlayer);
}

void ChessGameController::checkGameState()
{
    // 检查是否已经没有可移动的棋子位置了
    ChessPieceColor opponentColor = (m_currentPlayer == ChessPieceColor::Red) ?
                                        ChessPieceColor::Black : ChessPieceColor::Red;

    // 检查将军之后，棋子是否还可以移动，不可以移动表示“游戏结束”
    if (m_board->isCheckmate(opponentColor)) {
        m_gameOver = true;
        emit gameEnded(m_currentPlayer);
        return;
    }

    // 检查是否困毙（双方都无子可动）
    if (m_board->isStalemate()) {
        m_gameOver = true;
        emit gameEnded(ChessPieceColor::Empty); // 平局
        return;
    }

    // 检查是否被将军
    if (m_board->isCheck(opponentColor)) {
        // 可以在这里添加将军提示
        std::cout << (int)opponentColor << "被将军！";
    }
}
