#ifndef SENDFILE_H
#define SENDFILE_H

#include <QObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QtWebSockets/QtWebSockets>


class sendFile : public QWidget
{
    Q_OBJECT
public:
    explicit sendFile(QWidget *parent = nullptr);
    ~sendFile();

private:
    // 文件传输相关变量
    qint64 fileSize;
    qint64 bytesWritten;
    QString currentFileName;
    QByteArray fileBuffer;

    int progress;
    bool is_finished;

    QFile *currentFile;

public:
    int getProgress(){
        return this -> progress;
    }
    bool getIsFinished(){
        return this -> is_finished;
    }
    qint64 getFileSize(){
        return this -> fileSize;
    }
    QString getFileName(){
        return this -> currentFileName;
    }

    void setIsFinished(bool flag){
        this -> is_finished = flag;
    }

    void sendfile_(const QString &filePath,QWebSocket*socket);
    void sendFileChunk(QWebSocket * socket);
    void processFileRequest(const QByteArray &data,
                            QWebSocket*socket);

signals:
};

#endif // SENDFILE_H
