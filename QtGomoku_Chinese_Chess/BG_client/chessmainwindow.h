// chessmainwindow.h
#ifndef CHESSMAINWINDOW_H
#define CHESSMAINWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include "chessboardwidget.h"

class ChessMainWindow : public QDialog
{
    Q_OBJECT
public:
    explicit ChessMainWindow(const QString& playerName, QWidget *parent = nullptr);

    void closeEvent(QCloseEvent* event)override;

private slots:
    void onGameStarted();
    void onGameEnded(ChessPieceColor winner);
    void onNewGame();
    void onExit();

signals:
    void closed();
    void returnToSelection();

private:
    void setupUI();
    void updateStatus();

    ChessBoardWidget* m_boardWidget;
    QLabel* m_statusLabel;
    QPushButton* m_newGameBtn;
    QPushButton* m_exitBtn;
    QString m_playerName;
    ChessPieceColor m_humanColor;
};

#endif // CHESSMAINWINDOW_H
