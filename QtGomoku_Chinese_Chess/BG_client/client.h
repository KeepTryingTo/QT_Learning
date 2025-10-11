#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QInputDialog>
#include <QCheckBox>
#pragma once
#include <QTimer>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QMessageBox>
#include <QFile>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QSystemTrayIcon>

#include "board.h"
#include "player.h"
#include "piecetype.h"
#include "connectiondialog.h"
#include "networkmanager.h"
#include "ledindicator.h"
#include "chatdialog.h"
#include "onlineusersmanager.h"
#include "aigamecontroller.h"
#include "common.h"
#include "soundmanager.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT
public:
    // 获得棋盘的大小
    qreal getBoardWidth(){
        return this -> m_board_width;
    }
    void removePieceFromScene(int x, int y);

    QString getSocketStateString(QAbstractSocket::SocketState state);
    QString getLocalIp();
    void enableNetworkFeatures(bool enabled);

    void updateCurrentPlayerDisplay();
    void resetGameAfterEnd();
    void onOpponentExited();

    void updatePlayerDisplay();
    void displayWinner(const QString& winner);

    void handleGameOver(const QString& message);

    void drawPiece(int x, int y, PieceType player); // 绘制棋子
    void showGameResultDialog(const QString& winnerName);
    void showGameResult(const QString& result);
    void safelyExitAIGame();

    QString getUserName(){return m_player1Name;}

    // UI访问方法
    Ui::Client* getUI() const { return ui; }

    Board* getBoard(){
        return this->m_board;
    }
    Player* getPlayer1(){
        return this->m_player1;
    }
    Player* getPlayer2(){
        return this->m_player2;
    }

private slots:
    void recvClose();

    void on_regret_btn_clicked();

    void on_give_up_btn_clicked();

    void on_peace_btn_clicked();

    void on_pause_btn_clicked();

    void on_exit_game_btn_clicked();

    void on_conn_dialog_clicked();

    void updateGameTimer();           // 更新计时器

    void onPiecePlaced(int x, int y, PieceType pieceType);
    void onBoardCleared();

    void onGameResultButtonClicked(QAbstractButton* button);

    // 网络传输部分
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString &error);
    void onMoveReceived(int x, int y, PieceType piece);
    void onGameStartReceived(const QString &opponent, PieceType myPieceType, PieceType opponentPieceType);
    // void onGameStartReceived(const QString &opponent, PieceType myPieceType);
    void onGameEndReceived(const QString &winner);
    void onChatMessageReceived(const QString& sender, const QString &message);

    void onLoginSuccess(const QString &username);
    void onLoginFailed(const QString &error);
    void onLogoutReceived(const QString &username);
    void onUserStatusChanged(const QString &username, const QString &action);
    void onUserListReceived(const QStringList &users);

    void onChatMessageSent(const QString &message);

    void onChatActionTriggered();


    // 在线用户列表管理
    void showOnlineUsersDialog();
    void onUserDoubleClicked(const QString &username);
    void onRefreshUsersRequested();


    void onRoomCreated(const QString &roomId, const QString &roomName);
    void onRoomJoined(const QString &roomId, const QString &roomName, const QStringList &members);
    void onRoomLeft(const QString &roomId);
    void onRoomError(const QString &errorMessage);
    void onRoomListReceived(const QJsonArray &rooms);

    void onCreateRoom();
    void onJoinRoom();

    void onStatsUpdated(const QString &username, int score,
                                int wins, int losses, int draws, int streak);

    void recvResumStart();

    void onAIMadeMove(int x, int y);
    void exitToSelection();

    void RecvGiveUpInfo(const QString & username);
    void recvPeace();

    void onSoundSettingsTriggered();

public:
    Client(QString userName, QString gamename, QString gameMode, QWidget *parent = nullptr);
    ~Client();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void returnToSelection(); // 新增信号

private:
    Ui::Client *ui;

    QGraphicsScene* m_scene;

    int m_elapsedTime;

    // 游戏状态
    enum GameMode { PvP, PvE, EvE } m_gameMode;
    QGraphicsEllipseItem* m_lastMoveItem; // 标记最后一步的图形项

    // 玩家信息
    QString m_player1Name;
    QString m_player2Name;
    int m_player1Score;
    int m_player2Score;

    QString m_gameName;
    QString s_gameMode;

    // 设置选项
    bool m_soundEnabled;

    void setupUI();
    void initializeGame();
    void updatePlayerStats();
    void drawChessBoard();

    // 胜利结果展示函数
    void restartGame();
    void exitToMainMenu();
    void quitApplication();

    double calculateWinRate(int score, int totalGames) const;
    void highlightWinningLine();


    QPoint convertToBoardPosition(const QPoint& mousePos); // 将鼠标位置转换为棋盘坐标
    bool handleBoardClick(const QPoint& boardPos); // 处理棋盘点击
    bool checkWin(int x, int y, PieceType player);
    bool isBoardFull();

    // 认输比赛
    void handleSurrender();
    void showSurrenderDialog();

    // 比赛求和
    void handlePeaceRequest();
    void showPeaceRequestDialog();
    void processPeaceResponse(bool accepted);

    // 游戏状态
    enum GameState { Playing, Paused, Finished };
    GameState m_gameState;

    // 玩家信息
    Player* m_player1;        // 玩家1（通常是黑棋）
    Player* m_player2;        // 玩家2（通常是白棋）
    PieceType m_currentPlayer; // 1: player1, 2: player2

    // 游戏数据
    QVector<QVector<PieceType>> m_gameBoard; // 0:空, 1:黑棋, 2:白棋
    QList<QPoint> m_moveHistory;       // 落子历史记录

    // 界面控件（根据图片中的命名）
    QGraphicsView *graphicsView;
    QGraphicsScene *gameScene;

    QTimer *m_gameTimer;
    int elapsedTime; // 游戏进行时间（秒）
    int isPause;

    void updateTimer();

    // 棋盘绘制参数
    qreal m_boardStartX;
    qreal m_boardStartY;
    qreal m_board_width;
    qreal m_cellSize;

    Board * m_board;

    // 网络管理
    NetworkManager *m_networkManager;
    bool m_isConnected;

    // 登录状态
    bool m_isLoggedIn;
    QStringList m_onlineUsers;

    // 添加这些函数声明
    void updateOnlineUsersList();
    void updateLoginStatus();
    // void updateUserListWidget();
    void updateStatusBar();
    void addOnlineUser(const QString &username);
    // 可选：从在线列表移除用户
    void removeOnlineUser(const QString &username);
    // 可选：清空在线列表
    void clearOnlineUsers();

    // 显示灯
    LedIndicator* m_led;

    // IP和端口输入框
    // ConnectionDialog* m_conn_dialog;

    // 聊天
    ChatDialog* m_chatDialog;

    // 在线用户列表管理
    OnlineUsersManager *m_onlineUsersManager;

    // 房间ID和房间名称
    QString m_currentRoomId;
    QString m_currentRoomName;
    QListWidget *m_roomListWidget;

    // 用户列表
    QStringList m_members;

    // 当前用户
    QString m_currentOpponent;

    // 是否轮到当前用户下棋
    bool is_current_player;
    bool is_winner;

    // AI对决
    AIGameController* m_aiController;

    // 添加音效管理器
    SoundManager *m_soundManager;
};
#endif // CLIENT_H
