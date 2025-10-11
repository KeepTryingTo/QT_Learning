#include "board.h"
#include <QDebug>

Board::Board(int size, QObject *parent)
    : QObject(parent), m_size(size) {
    // QMutexLocker locker(&m_mutex);
    // 初始化棋盘网格
    m_grid.resize(m_size);
    for (auto &row : m_grid) {
        row.resize(m_size);
        row.fill(PieceType::Empty);
    }
}

// 实现拷贝构造函数
Board::Board(const Board& other)
{
    // QMutexLocker locker1(&m_mutex);
    // QMutexLocker locker2(&other.m_mutex);
    m_grid = other.m_grid;
}

Board& Board::operator=(const Board& other)
{
    if (this != &other) {
        // QMutexLocker locker1(&m_mutex);
        // QMutexLocker locker2(&other.m_mutex);
        m_grid = other.m_grid;
    }
    return *this;
}

// 复制棋盘状态
void Board::copyFrom(const Board& other)
{
    if (m_size != other.m_size) {
        qWarning() << "Board sizes don't match during copy";
        return;
    }

    m_grid = other.m_grid;
}

// 获取当前网格状态
QVector<QVector<PieceType>> Board::getGridState() const
{
    return m_grid;
}

// 设置网格状态
void Board::setGridState(const QVector<QVector<PieceType>>& grid)
{
    if (grid.size() == m_size && (!grid.isEmpty() && grid[0].size() == m_size)) {
        m_grid = grid;
    } else {
        qWarning() << "Invalid grid size during setGridState";
    }
}

// 判断当前坐标是否在棋盘的范围之内
bool Board::placePiece(int x, int y, PieceType piece) {
    // QMutexLocker locker(&m_mutex);
    // 检查坐标是否有效
    if (x < 0 || x >= m_size || y < 0 || y >= m_size) {
        qWarning() << "Invalid position:" << x << y;
        return false;
    }

    // 检查位置是否已有棋子
    if (m_grid[x][y] != PieceType::Empty) {
        qWarning() << "Position already occupied:" << x << y;
        return false;
    }

    // 放置棋子
    m_grid[x][y] = piece;

    // 发射信号通知棋子放置
    emit piecePlaced(x, y, piece);
    return true;
}

// 获得指定位置的棋子类型
PieceType Board::getPiece(int x, int y) const {
    // QMutexLocker locker(&m_mutex);
    // 检查坐标是否有效
    if (x < 0 || x >= m_size || y < 0 || y >= m_size) {
        qWarning() << "Invalid position:" << x << y;
        return PieceType::Empty;
    }

    return m_grid[x][y];
}


void Board::clear() {
    // QMutexLocker locker(&m_mutex);
    // 清空棋盘
    for (auto &row : m_grid) {
        row.fill(PieceType::Empty);
    }

    // 发射信号通知棋盘已清空
    emit boardCleared();
}

bool Board::checkWin(int x, int y, PieceType piece) const {
    // 检查坐标是否有效
    if (x < 0 || x >= m_size || y < 0 || y >= m_size) {
        return false;
    }

    // 检查四个方向是否五子连珠
    return checkDirection(x, y, 1, 0, piece) ||  // 水平
           checkDirection(x, y, 0, 1, piece) ||  // 垂直
           checkDirection(x, y, 1, 1, piece) ||  // 对角线（右下）
           checkDirection(x, y, 1, -1, piece);   // 对角线（右上）
}

bool Board::isFull() const {
    // 检查棋盘是否已满
    for (const auto &row : m_grid) {
        for (const auto &piece : row) {
            if (piece == PieceType::Empty) {
                return false;
            }
        }
    }
    return true;
}

bool Board::checkDirection(int x, int y, int dx, int dy, PieceType piece) const {
    int count = 1;  // 当前位置已有一个棋子

    // 正向检查
    for (int i = 1; i < 5; ++i) {
        int nx = x + i * dx;
        int ny = y + i * dy;

        if (nx < 0 || nx >= m_size || ny < 0 || ny >= m_size ||
            m_grid[nx][ny] != piece) {
            break;
        }
        count++;
    }

    // 反向检查
    for (int i = 1; i < 5; ++i) {
        int nx = x - i * dx;
        int ny = y - i * dy;

        if (nx < 0 || nx >= m_size || ny < 0 || ny >= m_size ||
            m_grid[nx][ny] != piece) {
            break;
        }
        count++;
    }

    return count >= 5;  // 五子连珠
}

QVector<QPoint> Board::getWinningLine() const {
    // 遍历整个棋盘寻找获胜连线
    for (int x = 0; x < m_size; ++x) {
        for (int y = 0; y < m_size; ++y) {
            PieceType piece = m_grid[x][y];
            if (piece == PieceType::Empty) continue;

            // 检查四个方向
            QVector<QPoint> winningLine;

            // 水平方向
            winningLine = getWinningLineInDirection(x, y, 1, 0, piece);
            if (!winningLine.isEmpty()) return winningLine;

            // 垂直方向
            winningLine = getWinningLineInDirection(x, y, 0, 1, piece);
            if (!winningLine.isEmpty()) return winningLine;

            // 右下对角线
            winningLine = getWinningLineInDirection(x, y, 1, 1, piece);
            if (!winningLine.isEmpty()) return winningLine;

            // 右上对角线
            winningLine = getWinningLineInDirection(x, y, 1, -1, piece);
            if (!winningLine.isEmpty()) return winningLine;
        }
    }

    return QVector<QPoint>(); // 没有获胜连线
}

QVector<QPoint> Board::getWinningLineInDirection(int startX, int startY, int dx, int dy, PieceType piece) const {
    QVector<QPoint> line;

    // 正向检查
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        int x = startX + i * dx;
        int y = startY + i * dy;

        if (x < 0 || x >= m_size || y < 0 || y >= m_size ||
            m_grid[x][y] != piece) {
            break;
        }

        line.append(QPoint(x, y));
        count++;
    }

    // 如果正向已经有5个，直接返回
    if (count >= 5) {
        return line;
    }

    // 反向检查（为了找到完整的连线）
    for (int i = 1; i < 5; ++i) {
        int x = startX - i * dx;
        int y = startY - i * dy;

        if (x < 0 || x >= m_size || y < 0 || y >= m_size ||
            m_grid[x][y] != piece) {
            break;
        }

        line.prepend(QPoint(x, y));
        count++;

        if (count >= 5) {
            return line;
        }
    }

    // 如果总数不足5个，清空返回
    if (count < 5) {
        line.clear();
    }

    return line;
}
