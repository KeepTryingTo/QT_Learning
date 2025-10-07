#ifndef ENEMY_H
#define ENEMY_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QTimer>

class Enemy : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    explicit Enemy(QPointF startPos, QGraphicsScene *scene, QObject *parent = nullptr);
    // 敌机也分不同的类型（容易，中等，最难）难度
    enum EnemyType { Easy, Medium, Hard };
    // 当前敌机的类型
    EnemyType getType() const;

    // 当前敌机的“生命值”
    void setHealth(int health){
        this -> health = health;
    }
    int getHealth(){
        return this -> health;
    }

    void stopShooting();
    void startShooting();
    void stopMovement();
    void startMovement();

public slots:
    void move();
    void shoot();

signals:
    void destroyed(Enemy *enemy);

private:
    QTimer *moveTimer;
    QTimer *shootTimer;
    EnemyType type;
    int speed;
    int health;
};

#endif // ENEMY_H
