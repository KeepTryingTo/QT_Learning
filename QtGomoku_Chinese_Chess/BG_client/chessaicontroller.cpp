// chessaicontroller.cpp
#include "chessaicontroller.h"
#include <QDebug>
#include <algorithm>
#include <random>

ChessAIController::ChessAIController(ChessBoard* board, QObject *parent)
    : QObject(parent)
    , m_board(board)
    , m_difficulty(3)  // 默认中等难度
    , m_aiColor(ChessPieceColor::Black)
{
}

void ChessAIController::setDifficulty(int difficulty)
{
    m_difficulty = qBound(1, difficulty, 5);
}

QPair<QPoint, QPoint> ChessAIController::findBestMove(ChessPieceColor aiColor)
{
    m_aiColor = aiColor;

    int depth;
    switch (m_difficulty) {
        case 1: depth = 1; break;
        case 2: depth = 2; break;
        case 3: depth = 3; break;
        case 4: depth = 4; break;
        case 5: depth = 5; break;
        default: depth = 3;
    }

    QVector<QPair<QPoint, QPoint>> allMoves = generateAllMoves(aiColor, *m_board);
    qDebug() << "生成的移动数量:" << allMoves.size();

    if (allMoves.isEmpty()) {
        qWarning() << "No valid moves found for AI";
        return QPair<QPoint, QPoint>();
    }

    if (m_difficulty == 1) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, allMoves.size() - 1);
        return allMoves[dis(gen)];
    }

    int bestScore = INT_MIN;
    QPair<QPoint, QPoint> bestMove;
    int validMoveCount = 0;

    for (const auto& move : allMoves) {
        ChessBoard tempBoard = *m_board;
        if (!tempBoard.movePiece(move.first.x(), move.first.y(), move.second.x(), move.second.y())) {
            qDebug() << "移动被拒绝: 从(" << move.first.x() << "," << move.first.y()
                << ") 到(" << move.second.x() << "," << move.second.y() << ")";
            continue;
        }
        // 如果可以移动的话，判断移动之后带来的价值，最后分数最大的作为移动的结果
        int score = minimax(depth - 1, INT_MIN, INT_MAX, false, tempBoard);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        validMoveCount++;
    }

    qDebug() << "实际有效的移动数量:" << validMoveCount;
    if (validMoveCount == 0) {
        qWarning() << "AI没有找到有效移动";
        return QPair<QPoint, QPoint>();
    }

    qDebug() << "最佳移动: 从(" << bestMove.first.x() << "," << bestMove.first.y()
             << ") 到(" << bestMove.second.x() << "," << bestMove.second.y()
             << "), 分数:" << bestScore;
    emit aiMoveReady(bestMove.first.x(), bestMove.first.y(), bestMove.second.x(), bestMove.second.y());
    return bestMove;
}


void ChessAIController::startThinking()
{
    qDebug() << "AI开始思考，当前颜色:" << (m_aiColor == ChessPieceColor::Red ? "红" : "黑");

    // 找到最佳位置开始移动
    QPair<QPoint, QPoint> bestMove = findBestMove(m_aiColor);

    if (!bestMove.first.isNull() && !bestMove.second.isNull()) {
        qDebug() << "AI决定移动: 从(" << bestMove.first.x() << "," << bestMove.first.y()
            << ") 到(" << bestMove.second.x() << "," << bestMove.second.y() << ")";

        emit aiMoveReady(bestMove.first.x(), bestMove.first.y(),
                         bestMove.second.x(), bestMove.second.y());
    } else {
        qWarning() << "AI没有找到有效移动";
    }
}

int ChessAIController::evaluateBoard(ChessBoard& currentBoard) const
{
    if (!m_board) return 0;

    // 棋子价值表
    const QHash<ChessPieceType, int> pieceValues = {
        {ChessPieceType::General, 10000},
        {ChessPieceType::Advisor, 200},
        {ChessPieceType::Elephant, 200},
        {ChessPieceType::Horse, 400},
        {ChessPieceType::Rook, 900},
        {ChessPieceType::Cannon, 450},
        {ChessPieceType::Pawn, 100}
    };

    int score = 0;

    // 计算棋盘上所有棋子的价值
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = m_board->getPiece(x, y);
            if (!piece.isEmpty()) {
                // 获得当前棋子对应的价值
                int value = pieceValues.value(piece.type, 0);

                // 添加位置权重（简单的 positional bonus）
                int positionalBonus = 0;

                // 根据不同棋子类型添加位置奖励
                switch (piece.type) {
                    case ChessPieceType::Pawn:
                        // 兵过河后价值增加
                        if ((piece.color == ChessPieceColor::Red && y >= 5) ||
                            (piece.color == ChessPieceColor::Black && y <= 4)) {
                            value += 50;
                        }
                        break;
                    case ChessPieceType::Horse:
                        // 马在中心位置更好
                        if (x >= 3 && x <= 5 && y >= 3 && y <= 6) {
                            positionalBonus += 10;
                        }
                        break;
                    case ChessPieceType::Cannon:
                        // 炮在对方底线更好
                        if ((piece.color == ChessPieceColor::Red && y <= 2) ||
                            (piece.color == ChessPieceColor::Black && y >= 7)) {
                            positionalBonus += 20;
                        }
                        break;
                    default:
                        break;
                }

                if (piece.color == m_aiColor) {
                    score += value + positionalBonus;
                } else {
                    score -= value + positionalBonus;
                }
            }
        }
    }

    // 检查将军状态
    ChessPieceColor opponentColor = (m_aiColor == ChessPieceColor::Red) ?
                                        ChessPieceColor::Black : ChessPieceColor::Red;

    if (m_board->isCheck(opponentColor)) {
        score += 50;  // 对方被将军，加分
    }

    if (m_board->isCheck(m_aiColor)) {
        score -= 50;  // 自己被将军，减分
    }

    return score;
}

int ChessAIController::minimax(int depth, int alpha, int beta, bool isMaximizing, ChessBoard& currentBoard)
{
    if (depth == 0) {
        return evaluateBoard(currentBoard);  // 修改为传参
    }

    // 终局检查
    if (currentBoard.isCheckmate(m_aiColor)) {
        return -100000;
    }
    if (currentBoard.isCheckmate(getOpponentColor(m_aiColor))) {
        return 100000;
    }
    if (currentBoard.isStalemate()) {
        return 0;
    }

    if (isMaximizing) {
        int maxEval = INT_MIN;
        QVector<QPair<QPoint, QPoint>> moves = generateAllMoves(m_aiColor, currentBoard);  // 修改为传参
        for (const auto& move : moves) {
            ChessBoard tempBoard = currentBoard;
            if (tempBoard.movePiece(move.first.x(), move.first.y(), move.second.x(), move.second.y())) {
                // 返回当前评估分数
                int eval = minimax(depth - 1, alpha, beta, false, tempBoard);
                // 最大评估分数
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                // 这里在执行一个剪枝过程，后面不会再有更大值的了，所以直接剪枝回到上一层
                if (beta <= alpha) break;
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        QVector<QPair<QPoint, QPoint>> moves = generateAllMoves(getOpponentColor(m_aiColor), currentBoard);
        for (const auto& move : moves) {
            ChessBoard tempBoard = currentBoard;
            // 假设移动当前棋子能带来的价值
            if (tempBoard.movePiece(move.first.x(), move.first.y(), move.second.x(), move.second.y())) {
                int eval = minimax(depth - 1, alpha, beta, true, tempBoard);
                // 最小分数
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                // 这里其实在执行一个剪枝的过程，后面不会再有更小值的了，所以直接剪枝回到上一层
                if (beta <= alpha) break;
            }
        }
        return minEval;
    }
}

ChessPieceColor ChessAIController::getOpponentColor(ChessPieceColor color) const
{
    return (color == ChessPieceColor::Red) ? ChessPieceColor::Black : ChessPieceColor::Red;
}


QVector<QPair<QPoint, QPoint>> ChessAIController::generateAllMoves(ChessPieceColor color, ChessBoard& currentBoard) const
{
    QVector<QPair<QPoint, QPoint>> allMoves;

    if (!m_board) {
        qWarning() << "棋盘指针为空!";
        return allMoves;
    }

    qDebug() << "生成" << (color == ChessPieceColor::Red ? "红方" : "黑方") << "的所有移动...";
    int pieceCount = 0;
    int moveCount = 0;

    // 遍历棋盘上的所有位置
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = m_board->getPiece(x, y);

            // 如果是当前颜色的棋子
            if (!piece.isEmpty() && piece.color == color) {
                pieceCount++;
                // qDebug() << "找到棋子:" << x << y << "类型:" << (int)piece.type;

                // 获取该棋子的所有合法移动
                QVector<QPoint> validMoves = m_board->getValidMoves(x, y);
                // qDebug() << "棋子(" << x << "," << y << ")有" << validMoves.size() << "个合法移动";

                for (const QPoint& target : validMoves) {
                    allMoves.append(qMakePair(QPoint(x, y), target));
                    moveCount++;
                }
            }
        }
    }

    qDebug() << "总共找到" << pieceCount << "个棋子，" << moveCount << "个合法移动";
    return allMoves;
}
