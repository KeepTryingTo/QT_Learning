#include "bullet.h"
#include <QGraphicsScene>
#include <QTimer>

Bullet::Bullet(QPointF startPos, QGraphicsScene *scene, int moveDist, int moveSpeed, Bullet::Direction direction,QObject *parent)
    : QObject(parent), speed(moveDist), moveSpeed(moveSpeed),bulletDirection(direction)
{
    setPixmap(QPixmap(":/resources/bullet.png").scaled(10, 20));
    setPos(startPos);

    moveTimer = new QTimer(this);
    // 定时16毫秒子弹移动一次
    connect(moveTimer, &QTimer::timeout, this, &Bullet::move);
    moveTimer->start(moveSpeed);
}

void Bullet::move()
{
    // 根据子弹方向移动
    if (bulletDirection == Up) {
        setPos(x(), y() - speed); // 玩家子弹向上移动
    } else {
        setPos(x(), y() + speed); // 敌机子弹向下移动
    }

    // 子弹超出场景范围时删除
    if ((bulletDirection == Up && y() + pixmap().height() < 0) ||
        (bulletDirection == Down && y() > scene()->height())) {
        scene()->removeItem(this);
        delete this;
    }
}

void Bullet::stopMovement()
{
    if (moveTimer && moveTimer->isActive()) {
        moveTimer->stop();
    }
}

void Bullet::startMovement()
{
    if (moveTimer && !moveTimer->isActive()) {
        moveTimer->start(moveSpeed); // 使用原来的移动速度
    }
}
