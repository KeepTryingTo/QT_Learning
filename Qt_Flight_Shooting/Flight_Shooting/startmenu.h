#ifndef STARTMENU_H
#define STARTMENU_H

// startmenu.h
#include <QDialog>
#include <QPushButton>
#include <QFile>
#include <QCloseEvent>

class StartMenu : public QDialog {
    Q_OBJECT
public:
    StartMenu(QWidget *parent = nullptr);

    void closeEvent(QCloseEvent *event) override;

signals:
    void startGame();  // 开始游戏信号
    void exitGame();   // 退出游戏信号

private:
    QPushButton *startButton;
    QPushButton *exitButton;
};

#endif // STARTMENU_H
