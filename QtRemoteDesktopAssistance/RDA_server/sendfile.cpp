#include "sendfile.h"

sendFile::sendFile(QWidget *parent)
    : QWidget{parent}
{
    this -> progress = 0;
    this -> fileSize = 0;
    this -> is_finished = false;
    this -> currentFile = nullptr;
    this -> bytesWritten = 0;
}
sendFile::~sendFile(){
    // 清理文件传输
    if (currentFile) {
        currentFile->close();
        delete currentFile;
        currentFile = nullptr;
    }
}

void sendFile::processFileRequest(const QByteArray &data,
                                  QWebSocket*socket)
{
    QDataStream stream(data);
    QString messageType;
    QString fileName;
    qint64 fileSize;

    stream >> messageType >> fileName >> fileSize;

    // 请求文件的时候服务端输出信息
    if (messageType == "FILE_REQUEST") {
        // ui->chat_frame->appendPlainText("📁 客户端请求文件: " + fileName +
        //                                 " (" + QString::number(fileSize) + " bytes)");

        // 这里可以添加文件存在性检查、权限验证等
        sendfile_(fileName, socket);
    }
}

void sendFile::sendfile_(const QString &filePath,
                         QWebSocket*socket)
{
    // 首先判断连接状态
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "没有客户端连接，无法发送文件");
        return;
    }

    // 获取文件信息
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误", "文件不存在: " + filePath);
        return;
    }

    // 清理之前的文件传输
    if (currentFile) {
        currentFile->close();
        delete currentFile;
    }

    // 创建文件对象，打开文件判断
    currentFile = new QFile(filePath);
    if (!currentFile->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
        delete currentFile;
        currentFile = nullptr;
        return;
    }
    // D:\\SoftwareFamily\\QT\\projects\\QtWebSocket\\websocket_server\\github-recovery-codes.txt

    // 当前文件大小以及文件名称
    fileSize = currentFile->size();
    bytesWritten = 0;
    currentFileName = fileInfo.fileName();

    // 发送文件头信息，仅仅是写入流中
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    // 将关键词“FILE_HEADER”添加到流中
    stream << QString("FILE_HEADER") << currentFileName << fileSize;

    // 发送文件头部信息二进制字节流
    socket->sendBinaryMessage(header);


    // 其次是发送具体文件信息，开始发送第一个数据块（分批次发送文件）
    sendFileChunk(socket);
}

void sendFile::sendFileChunk(QWebSocket * socket)
{
    if (!currentFile || !socket) return;

    const qint64 chunkSize = 64 * 1024; // 64KB 块大小
    QByteArray chunk = currentFile->read(chunkSize);

    if (chunk.isEmpty()) {
        // 文件发送完成
        currentFile->close();
        delete currentFile;
        currentFile = nullptr;

        is_finished = true;
        return;
    }

    // 发送数据块
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream << QString("FILE_CHUNK") << chunk;

    socket->sendBinaryMessage(message);
    bytesWritten += chunk.size();

    // 显示进度
    this -> progress = static_cast<int>((bytesWritten * 100) / fileSize);
}
