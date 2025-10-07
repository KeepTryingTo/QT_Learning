#include "playerspaceship.h"
#include "bullet.h"
#include <QGraphicsScene>
#include <QKeyEvent>

PlayerSpaceship::PlayerSpaceship(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), moveLeft(false), moveRight(false), speed(8)
{
    // 设置飞船图像
    setPixmap(QPixmap(":/resources/spaceship.png").scaled(50, 50));
    // 设置可获取焦点
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFocus();

    moveTimer = new QTimer(this);
    // 定时的更新飞船画面
    connect(moveTimer, &QTimer::timeout, this, &PlayerSpaceship::update);
    moveTimer->start(16);

    shootTimer = new QTimer(this);
    // 飞船定时的进行射击
    connect(shootTimer, &QTimer::timeout, this, &PlayerSpaceship::shoot);
    shootTimer->start(500); // 每500ms发射一次
}

void PlayerSpaceship::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        moveLeft = true;
        break;
    case Qt::Key_Right:
        moveRight = true;
        break;
    case Qt::Key_Space:
        shoot();
        break;
    case Qt::Key_Up:
        moveUp = true;
        break;
    case Qt::Key_Down:
        moveDown = true;
        break;
    }
}

void PlayerSpaceship::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        moveLeft = false;
        break;
    case Qt::Key_Right:
        moveRight = false;
        break;
    case Qt::Key_Up:
        moveUp = false;
        break;
    case Qt::Key_Down:
        moveDown = false;
        break;
    }
}

void PlayerSpaceship::update()
{
    // 左右移动
    if (moveLeft && x() > 0) {
        setPos(x() - speed, y());
    }
    if (moveRight && x() + pixmap().width() < scene()->width()) {
        setPos(x() + speed, y());
    }
    // 上下移动
    if (moveUp && y() > 0) {
        setPos(x(), y() - speed);
    }
    if (moveDown && y() + pixmap().height() < scene()->height()) {
        setPos(x(), y() + speed);
    }
}

void PlayerSpaceship::shoot()
{
    // 实例化子弹
    Bullet *bullet = new Bullet(QPointF(x() + pixmap().width() / 2 - 5, y()), scene(), 10, 16, Bullet::Up);
    scene()->addItem(bullet);

    // 播放射击音效
    // QMediaPlayer *shootSound = new QMediaPlayer;
    // QAudioOutput *audioOutput = new QAudioOutput;
    // shootSound->setAudioOutput(audioOutput);
    // shootSound->setSource(QUrl("qrc:/resources/shoot.wav"));
    // shootSound->play();

    // // 连接信号，在播放完成后删除对象
    // connect(shootSound, &QMediaPlayer::playbackStateChanged, [shootSound, audioOutput](QMediaPlayer::PlaybackState state) {
    //     if (state == QMediaPlayer::StoppedState) {
    //         shootSound->deleteLater();
    //         audioOutput->deleteLater();
    //     }
    // });
}

void PlayerSpaceship::stopShooting()
{
    if (shootTimer && shootTimer->isActive()) {
        shootTimer->stop();
    }
}

void PlayerSpaceship::startShooting()
{
    if (shootTimer && !shootTimer->isActive()) {
        shootTimer->start(500); // 恢复500ms的射击间隔
    }
}

void PlayerSpaceship::stopMovement()
{
    if (moveTimer && moveTimer->isActive()) {
        moveTimer->stop();
    }
}

void PlayerSpaceship::startMovement()
{
    if (moveTimer && !moveTimer->isActive()) {
        moveTimer->start(16);
    }
}
