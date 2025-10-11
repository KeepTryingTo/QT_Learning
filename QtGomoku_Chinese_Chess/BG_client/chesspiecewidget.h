// chesspiecewidget.h
#ifndef CHESSPIECEWIDGET_H
#define CHESSPIECEWIDGET_H

#include "piece.h"
#include <QWidget>

class ChessPieceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChessPieceWidget(const ChessPiece& piece, QWidget *parent = nullptr);

    void setPiece(const ChessPiece& piece);
    ChessPiece getPiece() const { return m_piece; }

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void clicked();

private:
    ChessPiece m_piece;
    bool m_selected;
    QFont m_chineseFont;
};

#endif // CHESSPIECEWIDGET_H
