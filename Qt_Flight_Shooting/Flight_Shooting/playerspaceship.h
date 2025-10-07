#ifndef PLAYERSPACESHIP_H
#define PLAYERSPACESHIP_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QKeyEvent>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>

class PlayerSpaceship : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    explicit PlayerSpaceship(QGraphicsScene *scene, QObject *parent = nullptr);
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    // 射击
    void shoot();

    void stopMovement();
    void startMovement();
    void stopShooting();
    void startShooting();

    // 添加生命值相关方法
    int getHealth() const { return health; }
    void setHealth(int value) { health = value; }
    void decreaseHealth(int amount = 1) { health -= amount; }

public slots:
    // 更新
    void update();

private:
    QTimer *moveTimer;
    QTimer *shootTimer;
    bool moveLeft;
    bool moveRight;
    bool moveUp;
    bool moveDown;
    int speed;

    // 初始化的生命值
    int health = 3;
};

#endif // PLAYERSPACESHIP_H
