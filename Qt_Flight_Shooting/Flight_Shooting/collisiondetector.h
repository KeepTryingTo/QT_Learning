#ifndef COLLISIONDETECTOR_H
#define COLLISIONDETECTOR_H

#include <QObject>
#include <QTimer>
#include "playerspaceship.h"
#include "enemy.h"
#include "bullet.h"

class CollisionDetector : public QObject
{
    Q_OBJECT

public:
    explicit CollisionDetector(QGraphicsScene *scene, QObject *parent = nullptr);
    void startDetection();
    void stopDetection();

signals:
    void gameOver();
    void scoreChanged(int newScore);  // 新增：分数变化信号


public slots:
    void checkCollisions();

private:
    QGraphicsScene *gameScene;
    QTimer *detectionTimer;
    int currentScore;
};

#endif // COLLISIONDETECTOR_H
