#include "flightshooting.h"
#include "ui_flightshooting.h"

/*
FlightShooting (主窗口)
├── GameScene (游戏场景)
├── PlayerSpaceship (玩家飞船)
├── EnemyController (敌人生成器)
├── Bullet (子弹类)
├── Enemy (敌人类)
├── CollisionDetector (碰撞检测器)
└── ScoreManager (分数管理器)
*/

FlightShooting::FlightShooting(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FlightShooting)
    , gameScene(nullptr)
    , player(nullptr)
    , enemyController(nullptr)
    , collisionDetector(nullptr)
    , isPaused(false)
    , finalScore(0)
{
    ui->setupUi(this);
    setWindowTitle("雷电游戏");
    hide(); // 初始时隐藏主窗口

    // 设置窗口标题和大小
    setWindowTitle("小蜜蜂游戏");
    // setFixedSize(1000, 700); // 根据您的界面调整大小

    // 初始化分数显示
    ui->score->setText("分数: 0");
    ui->score->setStyleSheet("font-size: 12px; font-weight: bold; color: white;");

    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }
}

FlightShooting::~FlightShooting()
{
    cleanupGame();
    delete ui;
}

void FlightShooting::updateScore(int newScore)
{
    ui->score->setText(QString("分数: %1").arg(newScore));
    finalScore = newScore;
}

void FlightShooting::onGameOver()
{
    qDebug() << "游戏结束处理开始";

    // 立即暂停所有游戏活动
    if (!isPaused) {
        pauseGame();
    }

    // 播放游戏结束音效
    if (gameOverSound) {
        gameOverSound->play();
        // 等待音效播放完成
        QEventLoop loop;
        connect(gameOverSound, &QMediaPlayer::playbackStateChanged, &loop, &QEventLoop::quit);
        QTimer::singleShot(2000, &loop, &QEventLoop::quit); // 最多等待2秒
        loop.exec();
    }

    // 显示游戏结束消息
    QMessageBox::information(this, "游戏结束",
                             QString("游戏结束！\n您的得分: %1").arg(finalScore));

    // 清理游戏资源
    cleanupGame();

    qDebug() << "游戏结束处理完成";
}



void FlightShooting::initializeGame()
{
    // 初始化游戏场景
    gameScene = new GameScene(this);
    std::cout<<ui->graphicsView->width()<<" "<<ui ->graphicsView->height()<<std::endl;
    gameScene->setSceneRect(0, 0, ui->graphicsView->width(), ui->graphicsView->height());

    ui->graphicsView->setScene(gameScene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 设置场景背景（根据您的网格背景）
    gameScene->setBackgroundBrush(QBrush(QColor(240, 240, 240), Qt::CrossPattern));

    // 初始化玩家
    player = new PlayerSpaceship(gameScene, this);
    gameScene->addItem(player);

    // 将玩家放置在底部中央
    QRectF sceneRect = gameScene->sceneRect();
    player->setPos(sceneRect.width() / 2 - player->boundingRect().width() / 2,
                   sceneRect.height() - player->boundingRect().height() - 10);

    // 初始化敌人生成器
    enemyController = new EnemyController(gameScene, this);

    // 初始化碰撞检测
    collisionDetector = new CollisionDetector(gameScene, this);

    // 连接信号
    connect(collisionDetector, &CollisionDetector::scoreChanged, this, &FlightShooting::updateScore);
    connect(collisionDetector, &CollisionDetector::gameOver, this, &FlightShooting::onGameOver);

    // 连接场景的键盘事件到玩家飞船
    connect(gameScene, &GameScene::keyPressed, player, &PlayerSpaceship::keyPressEvent);
    connect(gameScene, &GameScene::keyReleased, player, &PlayerSpaceship::keyReleaseEvent);

    // 开始游戏
    gameScene->startGame();
    enemyController->startSpawning();
    collisionDetector->startDetection();

    // 禁用开始按钮，避免重复点击
    ui->start_btn->setEnabled(false);

    ui->graphicsView->viewport()->update();
    // 初始化音效
    initializeSounds();

    // 开始播放背景音乐
    startBackgroundMusic();
}

void FlightShooting::initializeSounds()
{
    // 为每个音效创建独立的音频输出
    QAudioOutput *collisionAudioOutput = new QAudioOutput(this);
    collisionAudioOutput->setVolume(0.5);

    QAudioOutput *shootAudioOutput = new QAudioOutput(this);
    shootAudioOutput->setVolume(0.5);

    QAudioOutput *gameOverAudioOutput = new QAudioOutput(this);
    gameOverAudioOutput->setVolume(0.5);

    // 碰撞音效
    collisionSound = new QMediaPlayer(this);
    collisionSound->setAudioOutput(collisionAudioOutput);
    collisionSound->setSource(QUrl("qrc:/resources/explosion.wav"));

    // 射击音效
    shootSound = new QMediaPlayer(this);
    shootSound->setAudioOutput(shootAudioOutput);
    shootSound->setSource(QUrl("qrc:/resources/shoot.wav"));

    // 游戏结束音效
    gameOverSound = new QMediaPlayer(this);
    gameOverSound->setAudioOutput(gameOverAudioOutput);
    gameOverSound->setSource(QUrl("qrc:/resources/lose.wav"));

    // 背景音乐 - 单独的音量控制
    backgroundAudioOutput = new QAudioOutput(this);
    backgroundAudioOutput->setVolume(0.3);

    backgroundMusic = new QMediaPlayer(this);
    backgroundMusic->setAudioOutput(backgroundAudioOutput);
    backgroundMusic->setSource(QUrl("qrc:/resources/background_music.mp3"));
    backgroundMusic->setLoops(QMediaPlayer::Infinite);
}

void FlightShooting::startBackgroundMusic()
{
    if (backgroundMusic) {
        if (backgroundMusic->mediaStatus() == QMediaPlayer::NoMedia) {
            qDebug() << "背景音乐文件未加载";
            return;
        }
        if (backgroundMusic->mediaStatus() == QMediaPlayer::InvalidMedia) {
            qDebug() << "背景音乐文件格式不支持或路径错误";
            return;
        }
        backgroundMusic->play();
        qDebug() << "背景音乐开始播放，错误:" << backgroundMusic->error();
    }
}

void FlightShooting::stopBackgroundMusic()
{
    if (backgroundMusic) {
        backgroundMusic->stop();
    }
}

void FlightShooting::cleanupGame()
{
    qDebug() << "开始清理游戏资源";
    // 确保先停止所有定时器和活动
    if (collisionDetector) {
        collisionDetector->stopDetection();
    }

    if (enemyController) {
        enemyController->stopSpawning();
    }

    if (gameScene) {
        gameScene->stopGame();
    }

    // 处理场景中的项目
    if (gameScene) {
        // 先收集所有需要删除的项目
        QList<QGraphicsItem*> itemsToRemove;
        QList<QGraphicsItem*> items = gameScene->items();

        for (QGraphicsItem *item : items) {
            if (dynamic_cast<Enemy*>(item) || dynamic_cast<Bullet*>(item)) {
                itemsToRemove.append(item);
            }
        }

        // 批量删除项目
        for (QGraphicsItem *item : itemsToRemove) {
            gameScene->removeItem(item);
            delete item;
        }

        // 删除玩家
        if (player) {
            gameScene->removeItem(player);
            delete player;
            player = nullptr;
        }

        // 最后删除场景
        delete gameScene;
        gameScene = nullptr;
    }

    // 清理碰撞检测器
    if (collisionDetector) {
        delete collisionDetector;
        collisionDetector = nullptr;
    }

    // 清理敌人生成器
    if (enemyController) {
        delete enemyController;
        enemyController = nullptr;
    }

    // 重置暂停状态
    isPaused = false;

    // 重新启用开始按钮
    ui->start_btn->setEnabled(true);

    // 重置分数显示
    ui->score->setText("分数: 0");
    finalScore = 0;

    qDebug() << "游戏资源清理完成";
}

void FlightShooting::showGame() {
    show();
    activateWindow(); // 激活窗口
    raise(); // 将窗口提到最前面
}

void FlightShooting::on_start_btn_clicked()
{
    if (gameScene) {
        cleanupGame();
    }

    initializeGame();
}

void FlightShooting::on_close_btn_clicked()
{
    qDebug() << "关闭按钮点击，当前暂停状态:" << isPaused;

    // 如果游戏正在进行，先安全停止
    if (gameScene) {
        // 确保游戏不在暂停状态
        if (isPaused) {
            resumeGame();
            // 给游戏一点时间恢复
            QCoreApplication::processEvents();
            QThread::msleep(50);
        }

        // 安全清理游戏
        cleanupGame();
    }

    // 关闭窗口
    this->close();
}


void FlightShooting::on_pause_btn_clicked()
{
    if (!gameScene || !player) {
        return; // 游戏未开始，不执行暂停操作
    }
    if (isPaused) {
        resumeGame();
    } else {
        pauseGame();
    }
}

void FlightShooting::pauseGame()
{
    if (isPaused) return;
    isPaused = true;

    // 暂停背景音乐
    if (backgroundMusic && backgroundMusic->playbackState() == QMediaPlayer::PlayingState) {
        backgroundMusic->pause();
    }

    // 暂停所有游戏活动定时器
    if (gameScene) {
        gameScene->stopGame();
    }
    if (enemyController) {
        enemyController->stopSpawning();
    }
    if (collisionDetector) {
        collisionDetector->stopDetection();
    }
    if (player) {
        player->stopMovement();
        player->stopShooting(); // 新增：暂停玩家射击
    }

    // 暂停所有现有敌机和子弹
    QList<QGraphicsItem*> items = gameScene->items();
    for (QGraphicsItem *item : items) {
        if (Enemy *enemy = dynamic_cast<Enemy*>(item)) {
            enemy->stopMovement();
            enemy->stopShooting();
            // 确保敌机不会因为超出边界而被删除
            enemy->setData(0, "paused"); // 标记为暂停状态
        } else if (Bullet *bullet = dynamic_cast<Bullet*>(item)) {
            bullet->stopMovement();
        }
    }

    ui->pause_btn->setText("Go");
}
void FlightShooting::resumeGame()
{
    if (!isPaused) return;
    isPaused = false;

    // 恢复背景音乐
    if (backgroundMusic && backgroundMusic->playbackState() == QMediaPlayer::PausedState) {
        backgroundMusic->play();
    }

    // 清除所有敌机的暂停标记
    QList<QGraphicsItem*> items = gameScene->items();
    for (QGraphicsItem *item : items) {
        if (Enemy *enemy = dynamic_cast<Enemy*>(item)) {
            enemy->setData(0, ""); // 清除暂停标记
        }
    }

    // 恢复所有游戏活动
    if (gameScene) {
        gameScene->startGame();
    }
    if (enemyController) {
        enemyController->startSpawning();
    }
    if (collisionDetector) {
        collisionDetector->startDetection();
    }
    if (player) {
        player->startMovement();
        player->startShooting();
    }

    // 恢复所有敌机和子弹的移动
    for (QGraphicsItem *item : items) {
        if (Enemy *enemy = dynamic_cast<Enemy*>(item)) {
            enemy->startMovement();
            enemy->startShooting();
        } else if (Bullet *bullet = dynamic_cast<Bullet*>(item)) {
            bullet->startMovement();
        }
    }

    ui->pause_btn->setText("pause");
}
