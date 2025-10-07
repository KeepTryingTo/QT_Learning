#include "enemycontroller.h"
#include "enemy.h"
#include <QGraphicsScene>
#include <QRandomGenerator>

EnemyController::EnemyController(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene), spawnInterval(1000)
{
    spawnTimer = new QTimer(this);
    // 定时的控制产生敌机
    connect(spawnTimer, &QTimer::timeout, this, &EnemyController::spawnEnemy);
}

void EnemyController::startSpawning()
{
    // 默认1秒产生一个敌机
    spawnTimer->start(spawnInterval);
}

void EnemyController::stopSpawning()
{
    // 停止产生敌机
    spawnTimer->stop();

    // 清理所有敌人
    for (Enemy *enemy : enemies) {
        if (gameScene && enemy->scene() == gameScene) {
            gameScene->removeItem(enemy);
        }
        delete enemy;
    }
    enemies.clear();
}

void EnemyController::spawnEnemy()
{
    int sceneWidth = static_cast<int>(gameScene->width());
    int minX = 10;
    int maxX = sceneWidth - 10;

    // 确保范围有效
    if (maxX <= minX) {
        // 如果场景太小，使用安全值
        minX = 10;
        maxX = sceneWidth - 10;
        if (maxX <= minX) {
            // 场景实在太小，无法生成敌人
            return;
        }
    }

    // 敌机的产生位置
    int xPos = QRandomGenerator::global()->bounded(minX, maxX);
    // int xPos = QRandomGenerator::global()->bounded(50, static_cast<int>(gameScene->width()) - 50);
    // 最开始敌机是从最上面开始向下移动的，所以最开始Y轴距离为0
    Enemy *enemy = new Enemy(QPointF(xPos, 0), gameScene);
    // 敌机被击毁之后就释放对应的空间
    connect(enemy, &Enemy::destroyed, this, &EnemyController::removeEnemy);

    gameScene->addItem(enemy);
    enemies.append(enemy);
}

void EnemyController::removeEnemy(Enemy *enemy)
{
    enemies.removeOne(enemy);
    delete enemy;
}
