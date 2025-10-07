#include "gamescene.h"
#include <QDebug>

GameScene::GameScene(QObject *parent) : QGraphicsScene(parent)
{
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, [this]() {
        // 游戏逻辑更新
    });
}

void GameScene::startGame()
{
    // 屏幕刷新频率为60FPS
    gameTimer->start(16); // 约60FPS
}

void GameScene::stopGame()
{
    gameTimer->stop();
}

void GameScene::keyPressEvent(QKeyEvent *event)
{
    emit keyPressed(event);
    QGraphicsScene::keyPressEvent(event);
}

void GameScene::keyReleaseEvent(QKeyEvent *event)
{
    emit keyReleased(event);
    QGraphicsScene::keyReleaseEvent(event);
}
