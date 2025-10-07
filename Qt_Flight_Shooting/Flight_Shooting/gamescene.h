#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimer>

class GameScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GameScene(QObject *parent = nullptr);
    void startGame();
    void stopGame();

signals:
    void keyPressed(QKeyEvent *key);
    void keyReleased(QKeyEvent *key);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    QTimer *gameTimer;
};

#endif // GAMESCENE_H
