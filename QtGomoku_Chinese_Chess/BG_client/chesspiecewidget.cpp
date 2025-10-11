// chesspiecewidget.cpp
#include "chesspiecewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

ChessPieceWidget::ChessPieceWidget(const ChessPiece& piece, QWidget *parent)
    : QWidget(parent)
    , m_piece(piece)
    , m_selected(false)
{
    setFixedSize(60, 60); // 设置固定大小
    setAttribute(Qt::WA_TranslucentBackground); // 透明背景

    // 设置中文字体
    m_chineseFont.setFamily("SimSun");
    m_chineseFont.setPointSize(20);
    m_chineseFont.setBold(true);
}

void ChessPieceWidget::setPiece(const ChessPiece& piece)
{
    m_piece = piece;
    update(); // 重绘
}

void ChessPieceWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update(); // 重绘以显示选中状态
    }
}

void ChessPieceWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 使用固定尺寸，不要每次重新计算
    QRectF rect = this->rect();
    qreal diameter = qMin(rect.width(), rect.height()) * 0.8; // 固定为80%大小
    QPointF center = rect.center();

    if (!m_piece.isEmpty()) {
        // 绘制棋子背景
        QColor bgColor = (m_piece.color == ChessPieceColor::Red) ?
                             QColor(220, 20, 60) : QColor(0, 0, 0);

        painter.setBrush(bgColor);
        painter.setPen(QPen(Qt::black, 2));
        painter.drawEllipse(center, diameter/2, diameter/2);

        // 选中状态边框
        if (m_selected) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::blue, 3));
            painter.drawEllipse(center, diameter/2 + 2, diameter/2 + 2);
        }

        // 绘制棋子文字
        QFont font("SimSun", diameter * 0.4, QFont::Bold);
        painter.setFont(font);
        painter.setPen(Qt::white);

        QString pieceText;
        switch (m_piece.type) {
        case ChessPieceType::General: pieceText = (m_piece.color == ChessPieceColor::Red) ? "帅" : "将"; break;
        case ChessPieceType::Advisor: pieceText = (m_piece.color == ChessPieceColor::Red) ? "仕" : "士"; break;
        case ChessPieceType::Elephant: pieceText = (m_piece.color == ChessPieceColor::Red) ? "相" : "象"; break;
        case ChessPieceType::Horse: pieceText = "马"; break;
        case ChessPieceType::Rook: pieceText = "车"; break;
        case ChessPieceType::Cannon: pieceText = "炮"; break;
        case ChessPieceType::Pawn: pieceText = (m_piece.color == ChessPieceColor::Red) ? "兵" : "卒"; break;
        default: break;
        }

        painter.drawText(rect, Qt::AlignCenter, pieceText);
    }
}

void ChessPieceWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_piece.isEmpty()) {
            emit clicked();
            event->accept();
        }
    } else {
        event->ignore();
    }
}
