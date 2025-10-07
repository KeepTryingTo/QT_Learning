#include "collisiondetector.h"
#include "playerspaceship.h"
#include "enemy.h"
#include "bullet.h"
#include <QGraphicsScene>
#include <QDebug>
#include <QMediaPlayer>
#include <QAudioOutput>

CollisionDetector::CollisionDetector(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene)
{
    detectionTimer = new QTimer(this);
    connect(detectionTimer, &QTimer::timeout, this, &CollisionDetector::checkCollisions);
}

void CollisionDetector::startDetection()
{
    currentScore = 0;
    emit scoreChanged(currentScore);
    detectionTimer->start(16); // 约60FPS
}

void CollisionDetector::stopDetection()
{
    detectionTimer->stop();
}


void CollisionDetector::checkCollisions()
{
    // 检测子弹与敌人的碰撞（玩家子弹）
    QList<QGraphicsItem*> items = gameScene->items();
    for (QGraphicsItem *item : items) {
        if (Bullet *bullet = dynamic_cast<Bullet*>(item)) {
            QList<QGraphicsItem*> collidingItems = bullet->collidingItems();
            for (QGraphicsItem *collidingItem : collidingItems) {
                // 玩家子弹击中敌人
                Enemy *enemy = dynamic_cast<Enemy*>(collidingItem);
                PlayerSpaceship *player = dynamic_cast<PlayerSpaceship*>(collidingItem);
                if (bullet->getDirection() == Bullet::Up && enemy) {
                    // 播放碰撞音效
                    QMediaPlayer *collisionSound = new QMediaPlayer;
                    QAudioOutput *audioOutput = new QAudioOutput;
                    collisionSound->setAudioOutput(audioOutput);
                    collisionSound->setSource(QUrl("qrc:/resources/explosion.wav"));
                    collisionSound->play();

                    // 连接信号，在播放完成后删除对象
                    connect(collisionSound, &QMediaPlayer::playbackStateChanged, [collisionSound, audioOutput](QMediaPlayer::PlaybackState state) {
                        if (state == QMediaPlayer::StoppedState) {
                            collisionSound->deleteLater();
                            audioOutput->deleteLater();
                        }
                    });

                    // 处理子弹击中敌人
                    int health = enemy->getHealth();
                    health -= 1;
                    enemy->setHealth(health);
                    if (health <= 0) {
                        // 打掉不同敌机的难度获得分数不同
                        switch(enemy->getType()){
                            case Enemy::Easy:{
                                currentScore += 10;
                                break;
                            }
                            case Enemy::Medium:{
                                currentScore += 15;
                                break;
                            }
                            case Enemy::Hard:{
                                currentScore += 20;
                                break;
                            }
                        }
                        emit scoreChanged(currentScore); // 信息传递给主函数
                        emit enemy->destroyed(enemy);
                    }
                    // 子弹碰撞到战机之后移除
                    gameScene->removeItem(bullet);
                    delete bullet;
                    break;
                }
                // 敌机子弹击中玩家
                else if (bullet->getDirection() == Bullet::Down && player) {
                    // 播放碰撞音效
                    QMediaPlayer *collisionSound = new QMediaPlayer;
                    QAudioOutput *audioOutput = new QAudioOutput;
                    collisionSound->setAudioOutput(audioOutput);
                    collisionSound->setSource(QUrl("qrc:/resources/explosion.wav"));
                    collisionSound->play();

                    // 连接信号，在播放完成后删除对象
                    connect(collisionSound, &QMediaPlayer::playbackStateChanged,
                            [collisionSound, audioOutput](QMediaPlayer::PlaybackState state) {
                        if (state == QMediaPlayer::StoppedState) {
                            collisionSound->deleteLater();
                            audioOutput->deleteLater();
                        }
                    });

                    // 处理玩家被子弹击中（如果敌机启动了可以射击子弹的功能的话，我们这里默认关闭，不然场面有点混乱）
                    player->decreaseHealth();
                    if (player->getHealth() <= 0) {
                        emit gameOver();
                    }
                    gameScene->removeItem(bullet);
                    delete bullet;
                    break;
                }
            }
        }

        // 检测玩家与敌人的碰撞
        if (PlayerSpaceship *player = dynamic_cast<PlayerSpaceship*>(item)) {
            QList<QGraphicsItem*> collidingItems = player->collidingItems();
            for (QGraphicsItem *collidingItem : collidingItems) {
                if (Enemy *enemy = dynamic_cast<Enemy*>(collidingItem)) {
                    // 播放碰撞音效
                    QMediaPlayer *collisionSound = new QMediaPlayer;
                    QAudioOutput *audioOutput = new QAudioOutput;
                    collisionSound->setAudioOutput(audioOutput);
                    collisionSound->setSource(QUrl("qrc:/resources/explosion.wav"));
                    collisionSound->play();

                    // 连接信号，在播放完成后删除对象
                    connect(collisionSound, &QMediaPlayer::playbackStateChanged, [collisionSound, audioOutput](QMediaPlayer::PlaybackState state) {
                        if (state == QMediaPlayer::StoppedState) {
                            collisionSound->deleteLater();
                            audioOutput->deleteLater();
                        }
                    });

                    // 处理玩家与敌人碰撞（游戏结束）
                    emit gameOver();
                    break;
                }
            }
        }
    }
}
