#include "videoplayer.h"
#include "ui_videoplayer.h"

#include <QStyle>
#include <QTime>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("音频播放器");
    this -> setFixedSize(QSize(642,367));

    ui -> btn_start -> setIcon(style() -> standardIcon(QStyle::SP_MediaPlay));
    ui -> btn_pause -> setIcon(style() -> standardIcon(QStyle::SP_MediaPause));
    ui -> btn_stop -> setIcon(style() -> standardIcon(QStyle::SP_MediaStop));
    ui -> btn_seek_backward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekBackward));
    ui -> btn_seek_feedward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekForward));
    ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));
    ui -> btn_cycle_start -> setIcon(style() -> standardIcon(QStyle::SP_DialogResetButton));

    this -> mediaPlayer = new QMediaPlayer(this);//只涉及音频或者视频
    this -> audioOutput = new QAudioOutput;
    this -> mediaPlayer -> setAudioOutput(this -> audioOutput);
    this -> mediaPlayer -> setVideoOutput(ui -> videoWidget);

    this -> audioOutput -> setVolume(ui -> slider_volumn -> value());

    connect(this -> mediaPlayer,&QMediaPlayer::durationChanged,this,&VideoPlayer::durationChanged);
    connect(this -> mediaPlayer,&QMediaPlayer::positionChanged,this,&VideoPlayer::positionChanged);
    connect(ui -> spinBox,&QSpinBox::valueChanged,this -> audioOutput,&QAudioOutput::setVolume);

    ui -> slider_time -> setRange(0,this -> mediaPlayer -> duration() / 1000);

    //设置播放的速度
    float initSpeed = 1.0;
    ui -> comboBox -> addItem(QString::number(initSpeed));
    for(int i = 0 ; i < 4; i++){
        initSpeed = initSpeed + 0.25;
        ui -> comboBox -> addItem(QString::number(initSpeed));
    }
    initSpeed = 1.0;
    for(int i = 0 ; i < 3; i++){
        initSpeed = initSpeed - 0.25;
        ui -> comboBox -> addItem(QString::number(initSpeed));
    }
    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,&VideoPlayer::selectSpeed);

#if 0
    this -> mediaCaptureSession = new QMediaCaptureSession;//涉及音频捕获、处理、播放或录制的多个组件
    this -> mediaCaptureSession -> setVideoOutput(ui -> videoWidget);
    this -> mediaCaptureSession -> setAudioOutput(this -> audioOutput);
#endif

}

VideoPlayer::~VideoPlayer()
{
    delete ui;
}

void VideoPlayer::on_btn_close_clicked()
{
    this -> close();
}


void VideoPlayer::on_btn_open_file_clicked()
{
    QString video_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(video_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(video_path);
    this -> fileName = fileinformation.fileName();
    ui -> plainTextEdit -> appendPlainText(this -> fileName);

    qDebug() <<"the video path is: "<< video_path;
    this -> mediaPlayer -> setSource(QUrl::fromLocalFile(video_path));

    ui -> slider_volumn -> setMinimum(0);
    ui -> slider_volumn -> setMaximum(100);
    ui -> slider_volumn -> setValue(10);
}

void VideoPlayer::updateDuration(qint64 duration){
    QString timestr;
    if(duration || this -> Mduration){
        QTime CurrentTime((duration / 3600) % 60,(duration / 60) % 60,duration % 60,(duration * 1000) % 1000);
        QTime totalTime((Mduration / 3600) % 60,(Mduration / 60) % 60,Mduration % 60,(Mduration * 1000) % 1000);
        QString format = "mm:ss";
        if(this -> Mduration > 3600){
            format = "hh:mm:ss";
        }
        ui -> label_time_1 -> setText(CurrentTime.toString(format));
        ui -> label_time_2 -> setText(totalTime.toString(format));
    }
}

void VideoPlayer::durationChanged(qint64 duration){
    this -> Mduration = duration / 1000;
    //获得一个音频总时长
    ui -> slider_time -> setMaximum(this -> Mduration);
}

void VideoPlayer::positionChanged(qint64 progress){
    if(!ui -> slider_time -> isSliderDown()){
        ui -> slider_time -> setValue(progress / 1000);
    }
    this -> updateDuration(progress / 1000);
}

void VideoPlayer::selectSpeed(int index){
    for(int i = 0 ; i < ui -> comboBox -> count(); i++){
        if(index == i){
            this -> speed = ui -> comboBox ->itemText(index).toFloat();
            qDebug()<<"current speed: "<<this -> speed;
            this -> mediaPlayer -> setPlaybackRate(this -> speed);
            break;
        }
    }
}

void VideoPlayer::on_btn_seek_backward_clicked(){
    qint64 initTime = ui -> slider_time -> value();
    ui -> slider_time -> setValue(initTime - 10);
    this -> mediaPlayer -> setPosition(ui -> slider_time -> value() * 1000);
}

void VideoPlayer::on_btn_stop_clicked(){
    this -> mediaPlayer -> stop();
}

void VideoPlayer::on_btn_start_clicked(){
    this -> mediaPlayer -> play();
}

void VideoPlayer::on_btn_pause_clicked(){
    this -> mediaPlayer -> pause();
}

void VideoPlayer::on_btn_seek_feedward_clicked(){
    qint64 initTime = ui -> slider_time -> value();
    ui -> slider_time -> setValue(initTime + 10);
    this -> mediaPlayer -> setPosition(ui -> slider_time -> value() * 1000);
}

void VideoPlayer::on_btn_muted_clicked(){
    if(this -> is_muted == true){
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolumeMuted));
        this -> audioOutput -> setMuted(this -> is_muted);
        this -> is_muted = false;
    }else{
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));
        this -> audioOutput -> setMuted(this -> is_muted);
        this -> is_muted = true;
    }
}

void VideoPlayer::on_btn_cycle_start_clicked(){
    //设置无限循环播放
    this -> mediaPlayer -> setLoops(QMediaPlayer::Loops::Infinite);
    ui -> slider_time -> setValue(0);
    this -> updateDuration(0);
}

void VideoPlayer::on_slider_time_valueChanged(int value){
    this -> mediaPlayer -> setPosition(value * 1000);
}

void VideoPlayer::on_slider_volumn_valueChanged(int value){
    this -> audioOutput -> setVolume(value);
    ui -> spinBox -> setValue(value);
}

void VideoPlayer::showMetaDataDialog(){
    if (!m_metaDataDialog)
        m_metaDataDialog = new MetaDataDialog(this);
    //当这个Qt::WA_DeleteOnClose属性被设置到一个窗口或对话框上时，如果该窗口或对话框被关闭（无论是通过用户操作还是通过编程方式），
    //Qt将自动删除该窗口或对话框的实例。
    m_metaDataDialog->setAttribute(Qt::WA_DeleteOnClose, false);

    QMediaMetaData meta = this -> mediaPlayer -> metaData();
    for (int i = 0; i < QMediaMetaData::NumMetaData; i++) {
        auto key = static_cast<QMediaMetaData::Key>(i);
        if (i == QMediaMetaData::Date) {
            QDateTime currentTime = QDateTime::currentDateTime();
            this -> m_metaDataDialog -> m_metaDataFields[i] -> setText(currentTime.toString("yyyy-MM-dd HH:mm:ss"));
        }else{
            this -> m_metaDataDialog -> m_metaDataFields[i] -> setText(meta.stringValue(key));
        }
    }

    //当弹出的对话框按下OK按钮之后保存元数据,当调用这个Exec函数时，它会以模态方式显示对话框，并阻塞其他窗口的用户输入，直到当前对话框被关闭。
    if (m_metaDataDialog->exec() == QDialog::Accepted)
        return;
}

void VideoPlayer::on_btn_metaData_clicked()
{
    this -> showMetaDataDialog();
}


void VideoPlayer::on_btn_clear_clicked()
{
    ui -> plainTextEdit -> clear();
}

