#include "enemy.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "bullet.h"

Enemy::Enemy(QPointF startPos, QGraphicsScene *scene, QObject *parent)
    : QObject(parent), speed(3), health(1)
{
    // 随机选择敌机类型
    type = static_cast<EnemyType>(QRandomGenerator::global()->bounded(3));

    switch (type) {
    case Easy:
        setPixmap(QPixmap(":/resources/enemy_easy.png").scaled(40, 40));
        break;
    case Medium:
        setPixmap(QPixmap(":/resources/enemy_medium.png").scaled(45, 45));
        health = 2;
        break;
    case Hard:
        setPixmap(QPixmap(":/resources/enemy_hard.png").scaled(50, 50));
        health = 3;
        break;
    }

    // 敌机的初始位置
    setPos(startPos);

    moveTimer = new QTimer(this);
    // 定时移动敌机的位置
    connect(moveTimer, &QTimer::timeout, this, &Enemy::move);
    moveTimer->start(80);

    shootTimer = new QTimer(this);
    // 敌机定时的进行射击
    connect(shootTimer, &QTimer::timeout, this, &Enemy::shoot);
    shootTimer->start(2500); // 每2秒射击一次
}

Enemy::EnemyType Enemy::getType() const
{
    return type;
}

void Enemy::move()
{
    // 只有在定时器活动时才移动
    if (moveTimer && moveTimer->isActive()) {
        setPos(x(), y() + speed);

        // 只有在移动时才检测边界
        if (y() > scene()->height()) {
            emit destroyed(this);
        }
    }
}

void Enemy::shoot()
{
    // 实例化子弹，从敌机底部发射（向下移动）
    // Bullet *bullet = new Bullet(QPointF(x() + pixmap().width() / 2 - 5, y() + pixmap().height()), scene(),
    //                             10, 100, Bullet::Down);
    // scene()->addItem(bullet);

    // // 播放敌机射击音效（可以使用不同的音效）
    // QMediaPlayer *shootSound = new QMediaPlayer;
    // QAudioOutput *audioOutput = new QAudioOutput;
    // shootSound->setAudioOutput(audioOutput);
    // shootSound->setSource(QUrl("qrc:/resources/shoot.wav")); // 敌机专用射击音效
    // shootSound->play();

    // // 连接信号，在播放完成后删除对象
    // connect(shootSound, &QMediaPlayer::playbackStateChanged, [shootSound, audioOutput](QMediaPlayer::PlaybackState state) {
    //     if (state == QMediaPlayer::StoppedState) {
    //         shootSound->deleteLater();
    //         audioOutput->deleteLater();
    //     }
    // });
}

void Enemy::stopMovement()
{
    if (moveTimer && moveTimer->isActive()) {
        moveTimer->stop();
    }
}

void Enemy::startMovement()
{
    if (moveTimer && !moveTimer->isActive()) {
        moveTimer->start(80); // 恢复原来的80ms间隔
    }
}

void Enemy::stopShooting()
{
    if (shootTimer && shootTimer->isActive()) {
        shootTimer->stop();
    }
}

void Enemy::startShooting()
{
    if (shootTimer && !shootTimer->isActive()) {
        shootTimer->start(2500); // 恢复原来的2500ms间隔
    }
}
