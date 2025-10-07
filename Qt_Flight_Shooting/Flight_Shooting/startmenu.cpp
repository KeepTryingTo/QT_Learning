// startmenu.cpp
#include "startmenu.h"
#include <QVBoxLayout>
#include <QIcon>

StartMenu::StartMenu(QWidget *parent) : QDialog(parent) {
    setWindowIcon(QIcon(":/resources/game.png"));
    setWindowTitle("小蜜蜂游戏");
    setFixedSize(300, 200);

    // 按钮
    startButton = new QPushButton("进入游戏", this);
    exitButton = new QPushButton("退出游戏", this);

    // 设置按钮高度为 50px
    startButton->setFixedHeight(50);
    exitButton->setFixedHeight(50);

    // 增加按钮间距
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(startButton);
    layout->addSpacing(20);  // 按钮间距
    layout->addWidget(exitButton);
    layout->setContentsMargins(50, 30, 50, 30); // 边距（左,上,右,下）

    setLayout(layout);

    // 信号连接
    connect(startButton, &QPushButton::clicked, [this]() {
        emit startGame();
        accept(); // 使用 accept() 而不是 close()
    });

    connect(exitButton, &QPushButton::clicked, [this]() {
        emit exitGame();
        reject(); // 使用 reject() 而不是 close()
    });

    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }
}

void StartMenu::closeEvent(QCloseEvent *event) {
    emit exitGame();
    reject(); // 设置为拒绝结果
    event->accept();
}
