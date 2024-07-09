#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QAudioOutput>
#include <QFile>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("视频放器");
    this -> setFixedSize(QSize(550,450));
    this -> setWindowIcon(QIcon(":/new/prefix1/resources/images/icon.ico"));

    //音量图标
    // ui -> labStart -> setIcon(QIcon(":/new/prefix1/resources/images/sound.webp"));

    this -> player = new QMediaPlayer(this);
    this -> time = new QTime;

    this -> audioOutput = new QAudioOutput(this);
    QGraphicsScene * scene = new QGraphicsScene(this);
    ui -> graphicsView -> setScene(scene);

    //设置视频画布的大小
    this -> videoItems = new QGraphicsVideoItem;
    this -> videoItems -> setSize(QSize(490,285));
    this -> videoItems -> setFlags(QGraphicsItem::ItemIsMovable |
                               QGraphicsPathItem::ItemIsSelectable |
                               QGraphicsItem::ItemIsFocusable);

    scene -> addItem(this -> videoItems);
    //设置视频播放的音频输出到视频播放器中
    this -> player -> setAudioOutput(audioOutput);
    this -> player -> setVideoOutput(this -> videoItems);
    //设置无限循环播放
    this -> player -> setLoops(QMediaPlayer::Loops::Infinite);

    connect(this -> player,SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)),
            this,SLOT(onStateChanged(QMediaPlayer::PlaybackState)));
    connect(this -> player,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
    connect(this -> player,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));

    // connect(ui -> progressBar,SIGNAL(valueChanged(int)),this -> player,SLOT(setPosition(int)));
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::onStateChanged(QMediaPlayer::PlaybackState state){
    switch(state){
    case QMediaPlayer::PlayingState:
        ui -> stateEdit -> setText(this -> fileName + QString(": 正处于播放的状态"));
        break;
    case QMediaPlayer::PausedState:
        ui -> stateEdit -> setText(this -> fileName + QString(": 正处于暂停播放的状态"));
        break;
    case QMediaPlayer::StoppedState:
        ui -> stateEdit -> setText(this -> fileName + QString(": 停止播放"));
        break;
    default:
        break;
    }
}
//得到总的时长
void MainWindow::onDurationChanged(qint64 duration){
    this -> durationTime = QString::number((int)(duration / 1000));
    ui -> progressBar -> setMaximum((int)(duration / 1000));
    ui -> progressBar -> setMinimum(0);
    qDebug()<<"duration time: "<<duration;
}

void MainWindow::onPositionChanged(qint64 position){
    QString progress = QString::number((int)(position / 1000)) + "/" + this -> durationTime;
    // qDebug()<<"position: "<<position;
    ui -> labProgress -> setText(progress);
    ui -> progressBar -> setValue((int(position / 1000)));
}

void MainWindow::on_btnOpen_clicked(){
    QString video_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("MP4 Files(*.mp4);;All Files(*.*)"));
    if(video_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(video_path);
    this -> fileName = fileinformation.fileName();

    qDebug() <<"the video path is: "<< video_path;

    // this -> player -> setSource(QUrl::fromLocalFile("qrc:/new/prefix1/resources/video/DaWenXi.mp4"));
    this -> player -> setSource(QUrl::fromLocalFile(video_path));
    //计算1s的时间差，主要是为了开始画面的渲染，但是好像有问题
    // QTime startTime = QTime::currentTime();
    // this -> player -> play();
    // QTime endTime = QTime::currentTime();
    // int elapsed = startTime.msecsTo(endTime);
    // if(elapsed >= 1000){
    //     this -> player -> pause();
    // }
}

void MainWindow::on_btnStart_clicked(){
    this -> player -> play();
}

void MainWindow::on_btnStop_clicked(){
    this -> player -> pause();
}

void MainWindow::on_btnEnd_clicked(){
    this -> player -> stop();
}
//调节音量
void MainWindow::on_videoVolumn_valueChanged(int value){
    this -> audioOutput -> setVolume((float)value / 100);
    // qDebug()<<"音量: "<<(float)value / 100;
}

//当手动的改变进度条时，启动player的槽函数设置其值的位置
void MainWindow::on_progressBar_sliderMoved(int position){
    QString progress = QString::number(position) + "/" + this -> durationTime;
    ui -> labProgress -> setText(progress);
    ui -> progressBar -> setValue(position);
    this -> player -> setPosition((qint64)position*1000);
    qDebug()<<"position: "<<position;
}

