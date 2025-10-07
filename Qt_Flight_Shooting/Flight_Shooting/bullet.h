#ifndef BULLET_H
#define BULLET_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QTimer>

class Bullet : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    enum Direction { Up, Down };
    explicit Bullet(QPointF startPos, QGraphicsScene *scene, int moveDist, int moveSpeed,
                    Bullet::Direction direction,QObject *parent = nullptr);

    Direction getDirection() const { return bulletDirection; }
    void stopMovement();
    void startMovement();

public slots:
    void move();

private:
    QTimer *moveTimer;
    int speed;
    int moveSpeed;

    Direction bulletDirection;
};

#endif // BULLET_H
