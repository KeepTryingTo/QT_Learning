#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>

#include <QButtonGroup>
#include <drawingtool.h>
#include <QToolButton>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QGraphicsLineItem>

#include <iostream>

#include "filemanager.h"
#include "helpmanager.h"
#include "websocketmanager.h"
#include "connectdialog.h"
#include "roomdialog.h"
#include "ledindicator.h"
#include "chatdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();


    void updateColorDisplay(const QColor &color);
    HelpManager* getHelpManager() const { return m_helpManager; }
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    QString getLocalIp();
    QString getSocketStateString(QAbstractSocket::SocketState state);

private slots:
    void on_pen_btn_clicked();

    void on_line_btn_clicked();

    void on_elli_btn_clicked();

    void on_text_btn_clicked();

    void on_rect_btn_clicked();

    void on_zoom_big_btn_clicked();

    void on_zoom_small_btn_clicked();

    void onColorBtnClicked();

    void on_is_full_btn_clicked();

    void on_linew_spinBox_valueChanged(int arg1);

    void on_eraser_btn_clicked();

    // 简化的菜单槽函数
    void on_newFile();
    void on_openFile();
    void on_saveFile();
    void on_saveAsFile();
    void on_exportImage();
    void on_clearAction();  // 清除动作
    void on_redoAction();   // 重做动作
    void on_toggleGridAction();
    void onAboutTriggered();
    void onHelpTriggered();

    void onVersionTriggered();  // 版本信息
    void onCheckForUpdates();   // 检查更新

    void onUpdateAvailable(const QString &currentVersion, const QString &latestVersion);

    // 在线管理模块
    void onConnectedToServer();
    void onDisconnectedFromServer();
    void onConnectionError(const QString &error);
    void onDrawingOperationReceived(const DrawingOperation &operation);
    void onClearSceneReceived();
    void onJoinRoomClicked();
    void onConnectToServerClicked();

    // 添加缺失的方法实现
    void onDrawingOperationCreated(const DrawingOperation &operation);
    void onClearSceneRequested();
    void onUndoRequested();
    void onRedoRequested();
    void onUndoRequestReceived();
    void onRedoRequestReceived();
    void setOnlineMode(bool online);

    void disconnectClient();
    void recvError(const QString&errorMessage);

    void onChatActionTriggered();  // 聊天菜单项触发
    void onChatMessageSent(const QString &message);  // 发送聊天消息
    void onChatMessageReceived(const QString &senderId, const QString &message);  // 接收聊天消息

    void onUserInfoActionTriggered();  // 用户信息菜单点击
    void onClientListReceived(const QList<QJsonObject> &clients);
    void onUserJoined(const QString &userId, const QString &userName, int role);
    void onUserLeft(const QString &userId);

    void on_font_weight_valueChanged(int arg1);

    void onUndoRequestedWithData(const DrawingOperation &operation);
    void onRedoRequestedWithData(const DrawingOperation &operation);

private:
    Ui::Client *ui;
    DrawingTool * m_drawingTool;

    // 是否进行填充
    bool is_full;

    FileManager *m_fileManager;

    void updateGrid();  // 更新网格显示
    bool m_showGrid;    // 是否显示网格

    HelpManager *m_helpManager;  // 添加帮助管理器

    // 在线管理
    WebSocketManager *m_webSocketManager;
    bool m_isOnlineMode;
    QString m_currentRoomId;
    QString m_userName;

    LedIndicator *m_connectionStatus;

    // IP和端口
    QString ip;
    qint64 port;

    // 同一房间的不同客户端之前的聊天框
    ChatDialog *m_chatDialog;



    void setupUserListWidget();        // 初始化用户列表控件
    void updateUserListDisplay();      // 更新用户列表显示
    void toggleUserListVisibility();   // 切换用户列表显示状态

    QMap<QString, QJsonObject> m_currentRoomUsers; // 当前房间用户列表
    QListWidget *m_userListWidget;     // 用户列表显示控件
    QDockWidget *m_userListDock;       // 用户列表停靠窗口
    bool m_userListVisible;            // 用户列表是否可见
};
#endif // CLIENT_H
