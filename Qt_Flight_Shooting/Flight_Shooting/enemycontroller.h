#ifndef ENEMYCONTROLLER_H
#define ENEMYCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QGraphicsItem>
#include "enemy.h"

class EnemyController : public QObject
{
    Q_OBJECT

public:
    explicit EnemyController(QGraphicsScene *scene, QObject *parent = nullptr);
    // 敌机开始不断地增加
    void startSpawning();
    void stopSpawning();

public slots:
    void spawnEnemy();
    void removeEnemy(Enemy *enemy);

private:
    QGraphicsScene *gameScene;
    QTimer *spawnTimer;
    QList<Enemy*> enemies;
    int spawnInterval;
};

#endif // ENEMYCONTROLLER_H
