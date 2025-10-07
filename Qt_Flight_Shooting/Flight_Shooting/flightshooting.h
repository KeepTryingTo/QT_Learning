#ifndef FLIGHTSHOOTING_H
#define FLIGHTSHOOTING_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QThread>

#include <iostream>

#include "gamescene.h"
#include "playerspaceship.h"
#include "enemycontroller.h"
#include "scoremanager.h"
#include "collisiondetector.h"

#include "startmenu.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class FlightShooting;
}
QT_END_NAMESPACE

class FlightShooting : public QMainWindow
{
    Q_OBJECT

public:
    FlightShooting(QWidget *parent = nullptr);
    ~FlightShooting();

    void showGame();
    void startGame();
    void gameOver();

    void initializeSounds();

    void startBackgroundMusic();
    void stopBackgroundMusic();

private slots:
    void on_start_btn_clicked();
    void on_close_btn_clicked();

    void updateScore(int newScore);
    void onGameOver();

    void on_pause_btn_clicked();

    void pauseGame();
    void resumeGame();

private:
    Ui::FlightShooting *ui;

    GameScene *gameScene;
    PlayerSpaceship *player;
    EnemyController *enemyController;
    CollisionDetector *collisionDetector;

    void initializeGame();
    void cleanupGame();

    bool isPaused;
    int finalScore;

    // 添加音效相关成员
    QMediaPlayer *collisionSound;
    QMediaPlayer *shootSound;
    QMediaPlayer *gameOverSound;
    QAudioOutput *audioOutput;
    QMediaPlayer *backgroundMusic;
    QAudioOutput *backgroundAudioOutput;
};
#endif // FLIGHTSHOOTING_H
