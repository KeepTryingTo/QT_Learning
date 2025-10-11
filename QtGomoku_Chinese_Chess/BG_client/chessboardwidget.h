// chessboardwidget.h
#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include "chessgamecontroller.h"
#include <QWidget>

class ChessBoardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChessBoardWidget(QWidget *parent = nullptr);

    void initializeGame();

    ChessGameController* getGameController() const { return m_gameController; }
    QRectF getBoardRect() const;
    QPointF getPiecePosition(int x, int y) const;

signals:
    void moveMade(int fromX, int fromY, int toX, int toY);
    void gameStarted();
    void gameEnded(ChessPieceColor winner);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPieceMoved(int fromX, int fromY, int toX, int toY);
    void onGameStarted();
    void onGameEnded(ChessPieceColor winner);
    void onCurrentPlayerChanged(ChessPieceColor player);

private:
    QPoint convertToBoardPosition(const QPoint& mousePos);
    void drawBoard(QPainter& painter);
    void drawPieces(QPainter& painter);
    void drawSelectedSquare(QPainter& painter);
    void drawValidMoves(QPainter& painter);

    ChessGameController* m_gameController;
    QPoint m_selectedPos;
    QVector<QPoint> m_validMoves;
    qreal m_cellSize;
    qreal m_boardMargin;
};

#endif // CHESSBOARDWIDGET_H
