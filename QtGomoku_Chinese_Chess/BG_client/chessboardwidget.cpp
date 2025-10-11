// chessboardwidget.cpp
#include "chessboardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QDebug>

ChessBoardWidget::ChessBoardWidget(QWidget *parent)
    : QWidget(parent)
    , m_gameController(new ChessGameController(this))
    , m_selectedPos(-1, -1)
    , m_cellSize(0)
    , m_boardMargin(45)
{
    setMinimumSize(600, 700);  // 适当增加最小尺寸

    // 连接信号和槽
    connect(m_gameController, &ChessGameController::pieceMoved,this, &ChessBoardWidget::onPieceMoved);
    connect(m_gameController, &ChessGameController::gameStarted,this, &ChessBoardWidget::onGameStarted);
    connect(m_gameController, &ChessGameController::gameEnded,this, &ChessBoardWidget::onGameEnded);
    connect(m_gameController, &ChessGameController::currentPlayerChanged,this, &ChessBoardWidget::onCurrentPlayerChanged);

    connect(m_gameController, &ChessGameController::errorOccurred,this, [this](const QString& message) {
        QMessageBox::warning(this, "错误", message);
    });
}

// 在chessmainwindow中进入游戏之后进行初始化
void ChessBoardWidget::initializeGame()
{
    m_gameController->startGame(ChessPieceColor::Red);
    update(); // 调用paintEvent
}

/*
首次显示时​​：
    当控件第一次显示（如窗口创建、控件被添加到布局并显示）时，系统会自动触发 paintEvent。
​​内容需要更新时​​：
    当调用 update()或 repaint()时：
    ​​update()​​：
    将控件标记为“需要重绘”，Qt 会在下一个事件循环中合并多次 update()调用，优化性能（推荐使用）。
    ​​repaint()​​：
    立即强制重绘，可能影响性能（仅在需要实时响应时使用，如动画）。
​​窗口状态变化时​​：
    例如窗口被其他窗口遮挡后重新显示、窗口大小调整（resizeEvent后会自动触发重绘）。
​​显式调用 QWidget::update()时​​：
    其他代码主动请求重绘（例如数据变化后需要刷新界面）。
 */


// 初始化绘制棋盘
void ChessBoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制棋盘背景
    painter.fillRect(rect(), QColor(240, 217, 181)); // 米黄色背景

    // 计算棋盘参数
    qreal availableWidth = width() - 2 * m_boardMargin;
    qreal availableHeight = height() - 2 * m_boardMargin;
    m_cellSize = qMin(availableWidth / 8, availableHeight / 9);

    qreal boardWidth = 8 * m_cellSize;
    qreal boardHeight = 9 * m_cellSize;
    qreal startX = m_boardMargin + (availableWidth - boardWidth) / 2;
    qreal startY = m_boardMargin + (availableHeight - boardHeight) / 2;

    // 绘制棋盘网格
    drawBoard(painter);

    // 绘制选中的格子
    if (m_selectedPos.x() != -1) {
        drawSelectedSquare(painter);
    }

    // 绘制有效移动位置
    if (!m_validMoves.isEmpty()) {
        drawValidMoves(painter);
    }

    // 绘制棋子
    drawPieces(painter);
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 坐标转换
        QPoint boardPos = convertToBoardPosition(event->pos());

        // 坐标边界判断
        if (boardPos.x() >= 0 && boardPos.x() < 9 &&
            boardPos.y() >= 0 && boardPos.y() < 10) {

            if (m_selectedPos.x() == -1) {
                // 第一次点击：选择棋子
                ChessPiece piece = m_gameController->getBoard()->getPiece(boardPos.x(), boardPos.y());
                // 判断是否真的选中了棋子
                if (!piece.isEmpty() && piece.color == m_gameController->getCurrentPlayer()) {
                    m_selectedPos = boardPos;
                    // 根据当前点击的合法棋子，找到所有可移动的位置
                    m_validMoves = m_gameController->getBoard()->getValidMoves(boardPos.x(), boardPos.y());
                    update();
                }
            } else {
                // 第二次点击：移动棋子，首先判断有效的可以移动位置中是否有该棋子的位置
                if (m_validMoves.contains(boardPos)) {
                    emit moveMade(m_selectedPos.x(), m_selectedPos.y(),
                                  boardPos.x(), boardPos.y());

                    // 尝试移动
                    if (m_gameController->humanMove(m_selectedPos.x(), m_selectedPos.y(),
                                                    boardPos.x(), boardPos.y())) {
                        // 移动成功，清空选择，以及原来的位置也要置空
                        m_selectedPos = QPoint(-1, -1);
                        m_validMoves.clear();
                    }
                } else {
                    // 点击其他位置，重新选择
                    ChessPiece piece = m_gameController->getBoard()->getPiece(boardPos.x(), boardPos.y());
                    if (!piece.isEmpty() && piece.color == m_gameController->getCurrentPlayer()) {
                        m_selectedPos = boardPos;
                        m_validMoves = m_gameController->getBoard()->getValidMoves(boardPos.x(), boardPos.y());
                    } else {
                        m_selectedPos = QPoint(-1, -1);
                        m_validMoves.clear();
                    }
                }
                update();
            }
        }
    }
}

void ChessBoardWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update();
}

// 棋子移动之后，更新棋盘的布局
void ChessBoardWidget::onPieceMoved(int fromX, int fromY, int toX, int toY)
{
    Q_UNUSED(fromX);
    Q_UNUSED(fromY);
    Q_UNUSED(toX);
    Q_UNUSED(toY);
    update();
}

void ChessBoardWidget::onGameStarted()
{
    m_selectedPos = QPoint(-1, -1);
    m_validMoves.clear();
    emit gameStarted();
    update();
}

void ChessBoardWidget::onGameEnded(ChessPieceColor winner)
{
    QString winnerText;
    if (winner == ChessPieceColor::Red) {
        winnerText = "红方胜利！";
    } else if (winner == ChessPieceColor::Black) {
        winnerText = "黑方胜利！";
    } else {
        winnerText = "平局！";
    }

    QMessageBox::information(this, "游戏结束", winnerText);
    emit gameEnded(winner);
}

void ChessBoardWidget::onCurrentPlayerChanged(ChessPieceColor player)
{
    m_selectedPos = QPoint(-1, -1);
    m_validMoves.clear();

    QString playerText = (player == ChessPieceColor::Red) ? "红方回合" : "黑方回合";
    setToolTip(playerText);
    update();
}

QPoint ChessBoardWidget::convertToBoardPosition(const QPoint& mousePos)
{
    // 使用与棋盘相同的缩放比例
    qreal scaleFactor = 0.8;
    // 棋盘真正的高宽
    qreal availableWidth = (width() - 2 * m_boardMargin) * scaleFactor;
    qreal availableHeight = (height() - 2 * m_boardMargin) * scaleFactor;

    // 棋盘网格大小
    qreal cellSize = qMin(availableWidth / 8, availableHeight / 9);
    qreal boardWidth = 8 * cellSize;
    qreal boardHeight = 9 * cellSize;
    // 计算起始位置
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    // 检查是否在棋盘范围内
    if (mousePos.x() < startX || mousePos.y() < startY ||
        mousePos.x() > startX + boardWidth || mousePos.y() > startY + boardHeight) {
        return QPoint(-1, -1);
    }

    // 计算棋盘坐标
    int x = qRound((mousePos.x() - startX) / cellSize);
    int y = qRound((mousePos.y() - startY) / cellSize);

    // 确保坐标在有效范围内
    x = qBound(0, x, 8);
    y = qBound(0, y, 9);

    return QPoint(x, y);
}

// 绘制棋盘网格
void ChessBoardWidget::drawBoard(QPainter& painter)
{
    // 减小棋盘在窗口中的占比
    qreal availableWidth = width() - 2 * m_boardMargin;
    qreal availableHeight = height() - 2 * m_boardMargin;


    // 缩小棋盘比例：从原来的100%缩小到80%
    qreal scaleFactor = 0.8; // 调整这个值控制棋盘大小
    availableWidth *= scaleFactor;
    availableHeight *= scaleFactor;

    m_cellSize = qMin(availableWidth / 8, availableHeight / 9);

    // 重新计算棋盘位置，使其居中
    qreal boardWidth = 8 * m_cellSize;
    qreal boardHeight = 9 * m_cellSize;
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    // 确保棋盘不会太靠近边界
    startX = qMax(startX, m_boardMargin);
    startY = qMax(startY, m_boardMargin);

    // 绘制棋盘背景
    painter.fillRect(QRectF(startX, startY, boardWidth, boardHeight), QColor(210, 180, 140));

    // 绘制网格线 - 修正为10行9列（中国象棋标准）
    painter.setPen(QPen(Qt::black, 2));

    // 绘制横线（10条线）
    for (int i = 0; i < 10; ++i) {
        painter.drawLine(startX, startY + i * m_cellSize,
                         startX + boardWidth, startY + i * m_cellSize);
    }

    // 绘制竖线（9条线）
    for (int i = 0; i < 9; ++i) {
        if (i == 0 || i == 8) {
            // 边线贯穿整个棋盘
            painter.drawLine(startX + i * m_cellSize, startY,
                             startX + i * m_cellSize, startY + boardHeight);
        } else {
            // 中间线在楚河汉界处断开
            painter.drawLine(startX + i * m_cellSize, startY,
                             startX + i * m_cellSize, startY + 4 * m_cellSize);
            painter.drawLine(startX + i * m_cellSize, startY + 5 * m_cellSize,
                             startX + i * m_cellSize, startY + boardHeight);
        }
    }

    // 绘制九宫格斜线
    painter.setPen(QPen(Qt::black, 2));

    // 上方九宫格（黑方）
    painter.drawLine(startX + 3 * m_cellSize, startY,
                     startX + 5 * m_cellSize, startY + 2 * m_cellSize);
    painter.drawLine(startX + 5 * m_cellSize, startY,
                     startX + 3 * m_cellSize, startY + 2 * m_cellSize);

    // 下方九宫格（红方）
    painter.drawLine(startX + 3 * m_cellSize, startY + 7 * m_cellSize,
                     startX + 5 * m_cellSize, startY + 9 * m_cellSize);
    painter.drawLine(startX + 5 * m_cellSize, startY + 7 * m_cellSize,
                     startX + 3 * m_cellSize, startY + 9 * m_cellSize);

    // 绘制楚河汉界
    painter.setPen(QPen(Qt::black, 2));
    QFont font("SimSun", 12, QFont::Bold);
    painter.setFont(font);
    painter.drawText(QRectF(startX, startY + 4 * m_cellSize, boardWidth, m_cellSize),
                     Qt::AlignCenter, "楚河        汉界");
}


// 在棋盘上绘制棋子
void ChessBoardWidget::drawPieces(QPainter& painter)
{
    // 使用与棋盘相同的缩放比例和计算逻辑
    qreal scaleFactor = 0.8;
    qreal availableWidth = (width() - 2 * m_boardMargin) * scaleFactor;
    qreal availableHeight = (height() - 2 * m_boardMargin) * scaleFactor;

    qreal cellSize = qMin(availableWidth / 8, availableHeight / 9);
    qreal boardWidth = 8 * cellSize;
    qreal boardHeight = 9 * cellSize;
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    // 确保棋子完全在可视区域内
    startX = qMax(startX, m_boardMargin + 5);
    startY = qMax(startY, m_boardMargin + 5);

    if (!m_gameController || !m_gameController->getBoard()) return;

    QRectF boardRect = getBoardRect();
    cellSize = boardRect.width() / 8; // 棋子的大小
    qreal radius = cellSize * 0.35;

    // 绘制棋子上的字
    QFont pieceFont("SimSun", cellSize * 0.3, QFont::Bold);
    painter.setFont(pieceFont); // 字体

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            ChessPiece piece = m_gameController->getBoard()->getPiece(x, y);
            // 判断该位置是否为空
            if (piece.isEmpty()) continue;

            // 棋子的中心
            QPointF center = getPiecePosition(x, y);

            // 使用计算出的 startX, startY 和 cellSize
            qreal centerX = startX + x * cellSize;
            qreal centerY = startY + y * cellSize;

            // 绘制棋子背景
            QColor bgColor;
            if (piece.color == ChessPieceColor::Red) {
                bgColor = QColor(220, 20, 60);
            } else {
                bgColor = QColor(0, 0, 0);
            }

            painter.setBrush(bgColor);
            painter.setPen(QPen(Qt::black, 2));
            painter.drawEllipse(QPointF(centerX, centerY), radius, radius); // 绘制棋子是圆形

            // 绘制棋子文字是白色
            painter.setPen(Qt::white);
            QString pieceText;

            switch (piece.type) {
                case ChessPieceType::General: pieceText = (piece.color == ChessPieceColor::Red) ? "帅" : "将"; break;
                case ChessPieceType::Advisor: pieceText = (piece.color == ChessPieceColor::Red) ? "仕" : "士"; break;
                case ChessPieceType::Elephant: pieceText = (piece.color == ChessPieceColor::Red) ? "相" : "象"; break;
                case ChessPieceType::Horse: pieceText = "马"; break;
                case ChessPieceType::Rook: pieceText = "车"; break;
                case ChessPieceType::Cannon: pieceText = "炮"; break;
                case ChessPieceType::Pawn: pieceText = (piece.color == ChessPieceColor::Red) ? "兵" : "卒"; break;
                default: break;
            }

            QRectF textRect(centerX - radius, centerY - radius, radius * 2, radius * 2);
            painter.drawText(textRect, Qt::AlignCenter, pieceText);
        }
    }
}

void ChessBoardWidget::drawSelectedSquare(QPainter& painter)
{
    if (m_selectedPos.x() == -1) return;

    // 使用相同的计算逻辑
    qreal scaleFactor = 0.8;
    qreal availableWidth = (width() - 2 * m_boardMargin) * scaleFactor;
    qreal availableHeight = (height() - 2 * m_boardMargin) * scaleFactor;

    qreal cellSize = qMin(availableWidth / 8, availableHeight / 9);
    qreal boardWidth = 8 * cellSize;
    qreal boardHeight = 9 * cellSize;
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    // 关键修正：选中框应该围绕交叉点，而不是格子
    qreal centerX = startX + m_selectedPos.x() * cellSize;
    qreal centerY = startY + m_selectedPos.y() * cellSize;

    painter.setPen(QPen(Qt::blue, 3));
    painter.setBrush(Qt::NoBrush);

    // 绘制围绕交叉点的方框（就表示选中了当前的棋子）
    qreal squareSize = cellSize * 0.8;
    painter.drawRect(QRectF(centerX - squareSize/2, centerY - squareSize/2,
                            squareSize, squareSize));
}

// 用于绘制选中的当前棋子可以移动的其他表示（标记出来）
void ChessBoardWidget::drawValidMoves(QPainter& painter)
{
    if (m_validMoves.isEmpty()) return;

    // 使用与棋盘绘制相同的计算逻辑
    qreal scaleFactor = 0.8;
    qreal availableWidth = (width() - 2 * m_boardMargin) * scaleFactor;
    qreal availableHeight = (height() - 2 * m_boardMargin) * scaleFactor;

    qreal cellSize = qMin(availableWidth / 8, availableHeight / 9);
    qreal boardWidth = 8 * cellSize;
    qreal boardHeight = 9 * cellSize;
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    painter.setBrush(QColor(0, 255, 0, 100)); // 半透明绿色
    painter.setPen(Qt::NoPen);

    // 标记所有可以移动的位置（路径）
    for (const QPoint& move : m_validMoves) {
        // 关键修正：标记应该放在交叉点上，不是格子中间
        qreal centerX = startX + move.x() * cellSize;
        qreal centerY = startY + move.y() * cellSize;
        qreal markerSize = cellSize * 0.2; // 缩小标记尺寸

        // 绘制在交叉点位置
        painter.drawEllipse(QRectF(centerX - markerSize/2, centerY - markerSize/2,
                                   markerSize, markerSize));
    }
}

// 在类中添加一个辅助方法，统一计算棋盘坐标
QRectF ChessBoardWidget::getBoardRect() const
{
    qreal scaleFactor = 0.8;
    qreal availableWidth = (width() - 2 * m_boardMargin) * scaleFactor;
    qreal availableHeight = (height() - 2 * m_boardMargin) * scaleFactor;

    qreal cellSize = qMin(availableWidth / 8, availableHeight / 9);
    qreal boardWidth = 8 * cellSize;
    qreal boardHeight = 9 * cellSize;
    qreal startX = m_boardMargin + (width() - 2 * m_boardMargin - boardWidth) / 2;
    qreal startY = m_boardMargin + (height() - 2 * m_boardMargin - boardHeight) / 2;

    return QRectF(startX, startY, boardWidth, boardHeight);
}

QPointF ChessBoardWidget::getPiecePosition(int x, int y) const
{
    QRectF boardRect = getBoardRect();
    qreal cellSize = boardRect.width() / 8;

    return QPointF(boardRect.x() + x * cellSize, boardRect.y() + y * cellSize);
}
