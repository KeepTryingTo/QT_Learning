// chessboard.cpp
#include "chessboard.h"
#include <QDebug>
#include <QPoint>
#include <algorithm>

ChessBoard::ChessBoard(QObject *parent)
    : QObject(parent)
{
    initialize();
}

ChessBoard::ChessBoard(const ChessBoard& other)
    : QObject(other.parent())  // 调用父类构造函数
{
    // 手动复制棋盘数据
    m_board = other.m_board;
    // 复制其他必要成员变量
}

ChessBoard& ChessBoard::operator=(const ChessBoard& other)
{
    if (this != &other) {
        m_board = other.m_board;
        // 复制其他必要成员变量
    }
    return *this;
}

void ChessBoard::initialize()
{
    m_board.resize(10);
    for (int i = 0; i < 10; ++i) {
        m_board[i].resize(9);
        for (int j = 0; j < 9; ++j) {
            m_board[i][j] = ChessPiece();
        }
    }

    // 修正：红方在下方（y坐标较大），黑方在上方（y坐标较小）

    // 初始化黑方棋子（上方）
    m_board[0][0] = ChessPiece(ChessPieceType::Rook, ChessPieceColor::Black, 0, 0);
    m_board[0][1] = ChessPiece(ChessPieceType::Horse, ChessPieceColor::Black, 1, 0);
    m_board[0][2] = ChessPiece(ChessPieceType::Elephant, ChessPieceColor::Black, 2, 0);
    m_board[0][3] = ChessPiece(ChessPieceType::Advisor, ChessPieceColor::Black, 3, 0);
    m_board[0][4] = ChessPiece(ChessPieceType::General, ChessPieceColor::Black, 4, 0);
    m_board[0][5] = ChessPiece(ChessPieceType::Advisor, ChessPieceColor::Black, 5, 0);
    m_board[0][6] = ChessPiece(ChessPieceType::Elephant, ChessPieceColor::Black, 6, 0);
    m_board[0][7] = ChessPiece(ChessPieceType::Horse, ChessPieceColor::Black, 7, 0);
    m_board[0][8] = ChessPiece(ChessPieceType::Rook, ChessPieceColor::Black, 8, 0);

    m_board[2][1] = ChessPiece(ChessPieceType::Cannon, ChessPieceColor::Black, 1, 2);
    m_board[2][7] = ChessPiece(ChessPieceType::Cannon, ChessPieceColor::Black, 7, 2);

    m_board[3][0] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Black, 0, 3);
    m_board[3][2] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Black, 2, 3);
    m_board[3][4] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Black, 4, 3);
    m_board[3][6] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Black, 6, 3);
    m_board[3][8] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Black, 8, 3);

    // 初始化红方棋子（下方）
    m_board[9][0] = ChessPiece(ChessPieceType::Rook, ChessPieceColor::Red, 0, 9);
    m_board[9][1] = ChessPiece(ChessPieceType::Horse, ChessPieceColor::Red, 1, 9);
    m_board[9][2] = ChessPiece(ChessPieceType::Elephant, ChessPieceColor::Red, 2, 9);
    m_board[9][3] = ChessPiece(ChessPieceType::Advisor, ChessPieceColor::Red, 3, 9);
    m_board[9][4] = ChessPiece(ChessPieceType::General, ChessPieceColor::Red, 4, 9);
    m_board[9][5] = ChessPiece(ChessPieceType::Advisor, ChessPieceColor::Red, 5, 9);
    m_board[9][6] = ChessPiece(ChessPieceType::Elephant, ChessPieceColor::Red, 6, 9);
    m_board[9][7] = ChessPiece(ChessPieceType::Horse, ChessPieceColor::Red, 7, 9);
    m_board[9][8] = ChessPiece(ChessPieceType::Rook, ChessPieceColor::Red, 8, 9);

    m_board[7][1] = ChessPiece(ChessPieceType::Cannon, ChessPieceColor::Red, 1, 7);
    m_board[7][7] = ChessPiece(ChessPieceType::Cannon, ChessPieceColor::Red, 7, 7);

    m_board[6][0] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Red, 0, 6);
    m_board[6][2] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Red, 2, 6);
    m_board[6][4] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Red, 4, 6);
    m_board[6][6] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Red, 6, 6);
    m_board[6][8] = ChessPiece(ChessPieceType::Pawn, ChessPieceColor::Red, 8, 6);

    emit boardInitialized();
}

ChessPiece ChessBoard::getPiece(int x, int y) const
{
    if (x < 0 || x >= 9 || y < 0 || y >= 10) {
        return ChessPiece();
    }
    return m_board[y][x];
}

bool ChessBoard::movePiece(int fromX, int fromY, int toX, int toY)
{
    if (!isValidMove(fromX, fromY, toX, toY)) return false;

    ChessPiece piece = getPiece(fromX, fromY);
    ChessPiece movedPiece = piece;
    movedPiece.x = toX;  // 更新 x
    movedPiece.y = toY;  // 更新 y

    m_board[toY][toX] = movedPiece;
    m_board[fromY][fromX] = ChessPiece();

    return true;
}

// bool ChessBoard::movePiece(int fromX, int fromY, int toX, int toY)
// {
//     if (fromX < 0 || fromX >= 9 || fromY < 0 || fromY >= 10 ||
//         toX < 0 || toX >= 9 || toY < 0 || toY >= 10) {
//         return false;
//     }

//     ChessPiece fromPiece = m_board[fromY][fromX];
//     ChessPiece toPiece = m_board[toY][toX];

//     if (fromPiece.isEmpty()) {
//         return false;
//     }
//     // 检查移动的合法性
//     if (!isValidMove(fromX, fromY, toX, toY)) {
//         return false;
//     }

//     // 执行移动
//     m_board[toY][toX] = fromPiece;
//     m_board[toY][toX].x = toX;
//     m_board[toY][toX].y = toY;
//     m_board[fromY][fromX] = ChessPiece(); // 重新初始化

//     emit pieceMoved(fromX, fromY, toX, toY);

//     // 检查游戏是否结束
//     ChessPieceColor opponentColor = (fromPiece.color == ChessPieceColor::Red) ?
//                                         ChessPieceColor::Black : ChessPieceColor::Red;

//     if (isCheckmate(opponentColor)) {
//         emit gameOver(fromPiece.color);
//     } else if (isStalemate()) {
//         emit gameOver(ChessPieceColor::Empty); // 平局
//     }

//     return true;
// }

bool ChessBoard::isValidMove(int fromX, int fromY, int toX, int toY) const
{
    ChessPiece fromPiece = getPiece(fromX, fromY);
    ChessPiece toPiece = getPiece(toX, toY);

    if (fromPiece.isEmpty()) {
        return false;
    }

    // 不能吃自己的棋子
    if (!toPiece.isEmpty() && toPiece.color == fromPiece.color) {
        return false;
    }

    // 根据棋子类型检查移动是否合法
    switch (fromPiece.type) {
        case ChessPieceType::General:
            return isValidGeneralMove(fromPiece, toX, toY);
        case ChessPieceType::Advisor:
            return isValidAdvisorMove(fromPiece, toX, toY);
        case ChessPieceType::Elephant:
            return isValidElephantMove(fromPiece, toX, toY);
        case ChessPieceType::Horse:
            return isValidHorseMove(fromPiece, toX, toY);
        case ChessPieceType::Rook:
            return isValidRookMove(fromPiece, toX, toY);
        case ChessPieceType::Cannon:
            return isValidCannonMove(fromPiece, toX, toY);
        case ChessPieceType::Pawn:
            return isValidPawnMove(fromPiece, toX, toY);
        default:
            return false;
    }
}

bool ChessBoard::isCheckmate(ChessPieceColor color) const
{
    // 首先检查是否被将军
    if (!isCheck(color)) {
        return false;
    }

    // 检查是否有任何合法移动可以解除“将军”
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = getPiece(x, y);
            // 判断当前位置是否为空
            if (!piece.isEmpty() && piece.color == color) {
                // 获得当前位置所有合法可移动的位置
                QVector<QPoint> moves = getValidMoves(x, y);
                for (const QPoint& move : moves) {
                    // 模拟移动之后并检查是否仍然被将军
                    ChessBoard tempBoard = *this;
                    if (tempBoard.movePiece(x, y, move.x(), move.y())) {
                        if (!tempBoard.isCheck(color)) {
                            return false; // 有解将的移动
                        }
                    }
                }
            }
        }
    }

    return true; // 无解将的移动，被将死
}

bool ChessBoard::isCheck(ChessPieceColor color) const
{
    // 找到将/帅的位置
    QPoint generalPos(-1, -1);
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            // 判断是否为“将/帅”
            ChessPiece piece = getPiece(x, y);
            if (piece.type == ChessPieceType::General && piece.color == color) {
                generalPos = QPoint(x, y);
                break;
            }
        }
        if (generalPos.x() != -1) break;
    }

    if (generalPos.x() == -1) return false; // 将/帅不存在

    // 检查是否被对方任何棋子攻击
    ChessPieceColor opponentColor = (color == ChessPieceColor::Red) ?
                                        ChessPieceColor::Black : ChessPieceColor::Red;

    // 判断对方是否有可移动的位置
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = getPiece(x, y);
            // 对方的棋子是否可移动
            if (!piece.isEmpty() && piece.color == opponentColor) {
                if (isValidMove(x, y, generalPos.x(), generalPos.y())) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool ChessBoard::isStalemate() const
{
    // 检查双方是否都没有合法移动
    for (int color = 1; color <= 2; ++color) {
        ChessPieceColor pieceColor = (color == 1) ? ChessPieceColor::Red : ChessPieceColor::Black;

        bool hasValidMove = false;
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 9; ++x) {
                ChessPiece piece = getPiece(x, y);
                // 获得所有可以移动的位置
                if (!piece.isEmpty() && piece.color == pieceColor) {
                    if (!getValidMoves(x, y).isEmpty()) {
                        hasValidMove = true;
                        break;
                    }
                }
            }
            if (hasValidMove) break;
        }

        if (!hasValidMove && !isCheck(pieceColor)) {
            return true; // 困毙
        }
    }

    return false;
}

QVector<QPoint> ChessBoard::getValidMoves(int x, int y) const
{
    QVector<QPoint> validMoves;
    ChessPiece piece = getPiece(x, y);

    if (piece.isEmpty()) {
        return validMoves;
    }

    // 检查所有可能的目标位置
    for (int targetY = 0; targetY < 10; ++targetY) {
        for (int targetX = 0; targetX < 9; ++targetX) {
            // 判断当前移动是否合法，如果合法就保存当前位置
            if (isValidMove(x, y, targetX, targetY)) {
                validMoves.append(QPoint(targetX, targetY));
            }
        }
    }

    return validMoves;
}

QVector<QVector<ChessPiece>> ChessBoard::getBoardState() const
{
    return m_board;
}

bool ChessBoard::isValidGeneralMove(const ChessPiece& piece, int toX, int toY) const
{
    // 将/帅，只能向横或纵一个方向移动一格
    int dx = abs(toX - piece.x);
    int dy = abs(toY - piece.y);

    // 将/帅只能在九宫格内移动
    if (!isInPalace(toX, toY, piece.color)) {
        return false;
    }

    // 将/帅只能上下左右移动一格
    if ((dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
        // 检查对面将/帅是否在一条直线上且中间无棋子
        if (dx == 0) { // 垂直移动
            int startY = std::min(piece.y, toY);
            int endY = std::max(piece.y, toY);
            for (int y = startY + 1; y < endY; ++y) {
                if (!getPiece(piece.x, y).isEmpty()) {
                    return true; // 中间有棋子，不是对面将帅
                }
            }

            // 检查对面将帅
            for (int y = (piece.color == ChessPieceColor::Red) ? 0 : 9;
                 y >= 0 && y < 10;
                 y += (piece.color == ChessPieceColor::Red) ? 1 : -1) {
                ChessPiece targetPiece = getPiece(piece.x, y);
                if (targetPiece.type == ChessPieceType::General &&
                    targetPiece.color != piece.color) {
                    return true; // 对面将帅在一条线上
                }
                if (!targetPiece.isEmpty()) {
                    break; // 有其他棋子阻挡
                }
            }
        }
        return true;
    }

    return false;
}

bool ChessBoard::isValidAdvisorMove(const ChessPiece& piece, int toX, int toY) const
{
    int dx = abs(toX - piece.x);
    int dy = abs(toY - piece.y);

    // 士/仕只能在九宫格内移动
    if (!isInPalace(toX, toY, piece.color)) {
        return false;
    }

    // 士/仕只能斜着移动一格
    return (dx == 1 && dy == 1);
}

bool ChessBoard::isValidElephantMove(const ChessPiece& piece, int toX, int toY) const
{
    int dx = abs(toX - piece.x);
    int dy = abs(toY - piece.y);

    // 象/相不能过河
    if ((piece.color == ChessPieceColor::Red && toY < 5) ||
        (piece.color == ChessPieceColor::Black && toY > 4)) {
        return false;
    }

    // 象/相只能走田字
    if (dx != 2 || dy != 2) {
        return false;
    }

    // 检查象眼是否被塞
    int blockX = (piece.x + toX) / 2;
    int blockY = (piece.y + toY) / 2;
    // 检测目标位置是否空
    if (!getPiece(blockX, blockY).isEmpty()) {
        return false;
    }

    return true;
}

bool ChessBoard::isValidHorseMove(const ChessPiece& piece, int toX, int toY) const
{
    int dx = abs(toX - piece.x);
    int dy = abs(toY - piece.y);

    // 马走日字
    if (!((dx == 1 && dy == 2) || (dx == 2 && dy == 1))) {
        return false;
    }

    // 检查蹩马腿
    int blockX = piece.x;
    int blockY = piece.y;

    if (dx == 1) {
        blockY = piece.y + (toY > piece.y ? 1 : -1);
    } else {
        blockX = piece.x + (toX > piece.x ? 1 : -1);
    }
    // 检查蹩马腿的位置是否为空，为空才能移动
    if (!getPiece(blockX, blockY).isEmpty()) {
        return false;
    }

    return true;
}

bool ChessBoard::isValidRookMove(const ChessPiece& piece, int toX, int toY) const
{
    // 车只能直线移动
    if (piece.x != toX && piece.y != toY) {
        return false;
    }

    // 检查路径是否畅通
    return isPathClear(piece.x, piece.y, toX, toY);
}

bool ChessBoard::isValidCannonMove(const ChessPiece& piece, int toX, int toY) const
{
    // 炮只能直线移动，翻山
    if (piece.x != toX && piece.y != toY) {
        return false;
    }

    ChessPiece targetPiece = getPiece(toX, toY);
    int piecesInPath = 0;

    // 计算路径上的棋子数量
    if (piece.x == toX) { // 垂直移动
        int startY = std::min(piece.y, toY);
        int endY = std::max(piece.y, toY);
        for (int y = startY + 1; y < endY; ++y) {
            if (!getPiece(piece.x, y).isEmpty()) {
                piecesInPath++;
            }
        }
    } else { // 水平移动
        int startX = std::min(piece.x, toX);
        int endX = std::max(piece.x, toX);
        for (int x = startX + 1; x < endX; ++x) {
            if (!getPiece(x, piece.y).isEmpty()) {
                piecesInPath++;
            }
        }
    }

    if (targetPiece.isEmpty()) {
        // 移动时路径上不能有棋子
        return piecesInPath == 0;
    } else {
        // 吃子时路径上必须恰好有一个棋子作为炮架
        return piecesInPath == 1;
    }
}

bool ChessBoard::isValidPawnMove(const ChessPiece& piece, int toX, int toY) const
{
    int dx = abs(toX - piece.x);
    int dy = toY - piece.y;

    // 兵/卒只能向前/左/右移动，不能向后移动
    if (piece.color == ChessPieceColor::Red) {
        dy = -dy; // 红方兵向上移动
    }

    // 兵只能向前移动一格
    if (dy == 1 && dx == 0) {
        return true;
    }

    // 过河后可以横向移动一格
    if (isCrossRiver(piece.x, piece.color) && dy == 0 && dx == 1) {
        return true;
    }

    return false;
}

bool ChessBoard::isPathClear(int fromX, int fromY, int toX, int toY) const
{
    if (fromX == toX) { // 垂直移动
        int startY = std::min(fromY, toY);
        int endY = std::max(fromY, toY);
        for (int y = startY + 1; y < endY; ++y) {
            if (!getPiece(fromX, y).isEmpty()) {
                return false;
            }
        }
    } else { // 水平移动
        int startX = std::min(fromX, toX);
        int endX = std::max(fromX, toX);
        for (int x = startX + 1; x < endX; ++x) {
            if (!getPiece(x, fromY).isEmpty()) {
                return false;
            }
        }
    }

    return true;
}

bool ChessBoard::isInPalace(int x, int y, ChessPieceColor color) const
{
    if (color == ChessPieceColor::Red) {
        return (x >= 3 && x <= 5 && y >= 7 && y <= 9);
    } else {
        return (x >= 3 && x <= 5 && y >= 0 && y <= 2);
    }
}

bool ChessBoard::isCrossRiver(int y, ChessPieceColor color) const
{
    if (color == ChessPieceColor::Red) {
        return y < 5; // 红方过河（向上移动）
    } else {
        return y > 4; // 黑方过河（向下移动）
    }
}
