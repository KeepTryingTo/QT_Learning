#include "client.h"
#include "ui_client.h"

Client::Client(QString userName, QString gamename, QString gameMode, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);
    setFixedSize(QSize(615, 614));


    // 缺少 gameScene 的初始化！
    gameScene = new QGraphicsScene(this); // 需要添加这行
    ui->board->setScene(gameScene);

    m_player1Name = userName;
    s_gameMode = gameMode;
    m_gameName = gamename;
    if(s_gameMode == "pvp"){
        m_gameMode = PvP;
    }else if(s_gameMode == "pve"){
        m_gameMode = PvE;
    }

    if(m_gameName == "中国象棋" && m_gameMode == PvP){
        QMessageBox::information(this, "提示", "中国象棋的人人对战没有实现", QMessageBox::Yes | QMessageBox::No);
        return;
    }

    // 创建玩家对象
    m_player1 = new Player(userName, PieceType::Black, false); // 用户玩家，执黑
    if (gameMode == "pvp") {
        m_player2 = new Player("不在线", PieceType::White, false); // PvP 模式，对手是真人
    } else if (gameMode == "pve") {
        m_player2 = new Player("AI", PieceType::White, true);  // PvE 模式，对手是AI
    }

    QFile file(":/resources/board.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    ui->player_one->setText(QString("当前玩家: %1 (%2)")
                                .arg(m_player1->getName())
                                .arg("黑棋"));
    ui->player_two->setText(QString("对战玩家: %1 (%2)")
                                .arg(m_player2->getName())
                                .arg("白棋"));
    m_currentPlayer = PieceType::Black;
    initializeGame();

    // 游戏计时器
    ui->timeEdit->setDisplayFormat("hh:mm:ss");
    ui->timeEdit->setTime(QTime(0, 0, 0));
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(1000);
    connect(m_gameTimer, &QTimer::timeout, this, &Client::updateGameTimer);

    // 初始化 networkManager
    m_networkManager = new NetworkManager(this); // 添加这行
    m_isConnected = false;

    // 连接网络信号
    connect(m_networkManager, &NetworkManager::connected, this, &Client::onNetworkConnected);
    connect(m_networkManager, &NetworkManager::disconnected, this, &Client::onNetworkDisconnected);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &Client::onNetworkError);
    connect(m_networkManager, &NetworkManager::moveReceived, this, &Client::onMoveReceived);
    connect(m_networkManager, &NetworkManager::gameStartReceived, this, &Client::onGameStartReceived);
    connect(m_networkManager, &NetworkManager::gameEndReceived, this, &Client::onGameEndReceived);
    connect(m_networkManager, &NetworkManager::chatMessageReceived, this, &Client::onChatMessageReceived);
    connect(m_networkManager, &NetworkManager::sendResumeStart, this, &Client::recvResumStart);
    connect(m_networkManager, &NetworkManager::sendGiveUpsignal, this, &Client::RecvGiveUpInfo);
    connect(m_networkManager, &NetworkManager::sendPeaceSignal, this, &Client::recvPeace);

    m_led = new LedIndicator(this);
    m_led -> setStatus(LedIndicator::Off);
    // 添加到布局中
    layout()->addWidget(m_led);
    // 将指示灯设置为覆盖模式
    m_led->setParent(this);
    m_led->move(width() - m_led->width() - 10,height() - m_led->height() - 5);
    m_led->setStatus(LedIndicator::Off);


    // 在Client构造函数中添加
    connect(m_networkManager, &NetworkManager::loginSuccess, this, &Client::onLoginSuccess);
    connect(m_networkManager, &NetworkManager::loginFailed, this, &Client::onLoginFailed);
    connect(m_networkManager, &NetworkManager::logoutReceived, this, &Client::onLogoutReceived);
    connect(m_networkManager, &NetworkManager::userStatusChanged, this, &Client::onUserStatusChanged);

    // 在 Client 构造函数中添加连接
    connect(m_networkManager, &NetworkManager::userJoined, this, &Client::addOnlineUser);
    connect(m_networkManager, &NetworkManager::userLeft, this, &Client::removeOnlineUser);
    connect(m_networkManager, &NetworkManager::userListReceived, this, &Client::onUserListReceived);

    m_chatDialog = new ChatDialog(this);
    m_chatDialog->setUserName(m_player1Name);
    // 如果连接服务器成功之后，才能进行消息的发送
    connect(m_chatDialog, &ChatDialog::messageSent, this, &Client::onChatMessageSent);

    // 连接聊天菜单项
    connect(ui->chat, &QAction::triggered, this, &Client::onChatActionTriggered);
    connect(ui->users, &QAction::triggered, this, &Client::showOnlineUsersDialog);

    // 创建房间和加入房间
    connect(ui ->create_room, &QAction::triggered, this, &Client::onCreateRoom);
    connect(ui ->join_room, &QAction::triggered, this, &Client::onJoinRoom);

    // 在Client构造函数中添加
    connect(m_networkManager, &NetworkManager::roomCreated, this, &Client::onRoomCreated);
    connect(m_networkManager, &NetworkManager::roomJoined, this, &Client::onRoomJoined);
    connect(m_networkManager, &NetworkManager::roomLeft, this, &Client::onRoomLeft);
    connect(m_networkManager, &NetworkManager::roomListReceived, this, &Client::onRoomListReceived);
    connect(m_networkManager, &NetworkManager::roomError, this, &Client::onRoomError);

    m_onlineUsersManager = new OnlineUsersManager(this);
    // 连接管理器的信号
    connect(m_onlineUsersManager, &OnlineUsersManager::userDoubleClicked, this, &Client::onUserDoubleClicked);
    connect(m_onlineUsersManager, &OnlineUsersManager::refreshRequested, this, &Client::onRefreshUsersRequested);
    connect(m_onlineUsersManager, &OnlineUsersManager::userCountChanged,
            this, [this](int count) {
                if (statusBar()) {
                    statusBar()->showMessage(QString("在线用户: %1 人").arg(count));
                }
            });

    // 隔5秒就向服务器请求
    QTimer* m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [&](){
        if (m_networkManager && m_networkManager->isConnected()) {
            m_networkManager->requestUserList();
        }
        for(int i = 0; i < m_members.size(); i++){
            if(m_members[i] != m_player1->getName()){
                ui->player_two->setText(QString("对战玩家: %1 (%2)")
                                            .arg(m_members[i])
                                            .arg(m_player2->getPieceType() == PieceType::Black ? "黑棋" : "白棋"));
                m_currentOpponent = m_members[i];
            }
            addOnlineUser(m_members[i]);
        }
    });
    m_timer->setInterval(5000);//5秒
    m_timer->start();

    // 传输输赢结果
    connect(m_networkManager, &NetworkManager::statsUpdated, this, &Client::onStatsUpdated);

    // 在Client构造函数中初始化
    m_aiController = new AIGameController(this, this);

    if(s_gameMode == "pve"){
        m_aiController->startGame();
    }

    // 连接信号
    connect(m_aiController, &AIGameController::aiMadeMove, this, &Client::onAIMadeMove);
    connect(m_aiController, &AIGameController::exitToSelection, this, &Client::exitToSelection); // 退出游戏信号

    // 初始化音效管理器
    m_soundManager = new SoundManager(this);

    connect(ui->soundSettings, &QAction::triggered, this, &Client::onSoundSettingsTriggered);
}

Client::~Client()
{
    delete m_networkManager;
    delete m_chatDialog;
    delete m_led;
    delete m_player1;
    delete m_player2;
    delete ui;
}

// 在client.cpp中实现
void Client::onAIMadeMove(int x, int y)
{
    if (!m_aiController) {
        return;
    }

    // 获取AI的棋子类型
    PieceType aiPiece = m_aiController->getAIPieceType();

    // 在棋盘上放置棋子
    if (m_board->placePiece(x, y, aiPiece)) {
        // 绘制棋子
        drawPiece(x, y, aiPiece);

        // 添加到历史记录
        m_moveHistory.append(QPoint(x, y));

        // 检查游戏状态
        if (m_board->checkWin(x, y, aiPiece)) {
            // AI获胜时的积分统计
            if (m_player2 && m_player2->isAI()) {
                m_player2->addWin();
                m_player2->addScore(10);
                m_player1->addLoss();
                updatePlayerStats(); // 更新UI显示
            }
            handleGameOver("AI获胜！");
        } else if (m_board->isFull()) {
            // 平局
            handleGameOver("平局！");
        } else {
            // 切换当前玩家到人类
            m_currentPlayer = (aiPiece == PieceType::Black) ? PieceType::White : PieceType::Black;
            updatePlayerDisplay();

            // 启用悔棋按钮
            ui->regret_btn->setEnabled(true);

            // 更新状态栏
            ui->statusBar->showMessage("轮到您落子");
        }
    }
}

// 辅助方法：处理游戏结束
void Client::handleGameOver(const QString& message)
{
    // 停止计时器
    if (m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 更新游戏状态
    // m_gameState = GameOver;

    // 显示游戏结果
    showGameResult(message);

    // 更新玩家统计
    updatePlayerStats();

    if(s_gameMode == "pve"){
        m_aiController->startGame();
    }

    // 禁用操作按钮
    ui->regret_btn->setEnabled(false);
}

// 和AI对决退出游戏槽函数处理
void Client::exitToSelection()
{
    // 停止所有计时器
    if (m_gameTimer && m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 清理AI控制器
    if (m_aiController) {
        m_aiController->exitGame();
    }

    // 重置游戏状态
    // m_gameState = NotStarted;

    // 清空棋盘
    if (m_board) {
        m_board->clear();
    }
    gameScene->clear();
    drawChessBoard();

    // 清空历史记录
    m_moveHistory.clear();

    // 重置计时器显示
    elapsedTime = 0;
    ui->timeEdit->setTime(QTime(0, 0, 0));

    // 更新UI状态
    ui->regret_btn->setEnabled(false);
    ui->pause_btn->setEnabled(false);

    // 更新状态栏
    ui->statusBar->showMessage("游戏已退出，等待开始新游戏");

    // 本地处理退出到游戏选择界面
    exitToMainMenu();
}


void Client::initializeGame() {
    m_gameState = Playing;

    m_player2Name = "tom";

    is_winner = false;

    elapsedTime = 0;
    isPause = 0;

    // 重置玩家分数
    m_player1->resetScore();
    m_player2->resetScore();

    m_board = new Board(15);
    m_board_width = 15;
    m_board->clear();


    // 连接 Board 的信号
    connect(m_board, &Board::piecePlaced, this, &Client::onPiecePlaced);
    connect(m_board, &Board::boardCleared, this, &Client::onBoardCleared);

    // 初始化棋盘（15x15）
    m_gameBoard = QVector<QVector<PieceType>>(15, QVector<PieceType>(15, PieceType::Empty));
    m_moveHistory.clear();

    // 初始化场景
    if (!gameScene) {
        gameScene = new QGraphicsScene(this);
        ui->board->setScene(gameScene); // 确保设置场景到board
    }

    // 清空场景并绘制棋盘
    gameScene->clear();
    drawChessBoard();
}

void Client::drawChessBoard() {
    // 获取 board 组件的尺寸
    QGraphicsView* boardView = ui->board; // 假设你的QGraphicsView对象名为board
    QRectF boardRect = boardView->rect();

    // 计算棋盘的实际可用大小（留出边距）
    qreal margin = 20; // 边距
    qreal availableWidth = boardRect.width() - 2 * margin;
    qreal availableHeight = boardRect.height() - 2 * margin;

    // 确定单元格大小，确保棋盘是正方形
    qreal cellSize = qMin(availableWidth, availableHeight) / 14; // 14个间隔（15个点）

    // 计算棋盘起始位置（居中显示）
    qreal startX = margin + (availableWidth - 14 * cellSize) / 2;
    qreal startY = margin + (availableHeight - 14 * cellSize) / 2;

    // 清空场景重新绘制
    gameScene->clear();

    // 绘制棋盘背景
    QBrush boardBrush(QColor(220, 179, 92));
    gameScene->addRect(startX, startY, 14 * cellSize, 14 * cellSize,
                       QPen(Qt::black, 2), boardBrush);

    // 绘制棋盘网格（15x15）
    for (int i = 0; i < 15; ++i) {
        // 横线
        gameScene->addLine(startX, startY + i * cellSize,
                           startX + 14 * cellSize, startY + i * cellSize,
                           QPen(Qt::black, 1));
        // 竖线
        gameScene->addLine(startX + i * cellSize, startY,
                           startX + i * cellSize, startY + 14 * cellSize,
                           QPen(Qt::black, 1));
    }

    // 绘制星位
    QBrush starBrush(Qt::black);
    int starPositions[5][2] = {{3,3}, {3,11}, {7,7}, {11,3}, {11,11}};
    for (int i = 0; i < 5; ++i) {
        gameScene->addEllipse(startX + starPositions[i][0] * cellSize - 3,
                              startY + starPositions[i][1] * cellSize - 3,
                              6, 6, QPen(), starBrush);
    }

    // 存储棋盘参数供后续使用
    m_boardStartX = startX;
    m_boardStartY = startY;
    m_cellSize = cellSize;
}

void Client::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    // 重新绘制棋盘以适应新的大小
    if (gameScene) {
        drawChessBoard();
    }
}

// 鼠标点击事件处理
void Client::mousePressEvent(QMouseEvent* event) {

    if (event->button() == Qt::LeftButton) {
        if (s_gameMode == "pve" && m_aiController && m_aiController->getIsAITurn()) {
            QMessageBox::information(this, "提示", "请等待AI落子");
            return;
        }

        // 将鼠标位置转换为棋盘坐标
        QPoint boardPos = convertToBoardPosition(event->pos());

        // 检查是否在棋盘范围内
        if (boardPos.x() >= 0 && boardPos.x() < 15 &&
            boardPos.y() >= 0 && boardPos.y() < 15) {
            bool is_success = handleBoardClick(boardPos);

            // 下棋必须落子成功才能轮到AI下棋
            if (is_success && s_gameMode == "pve" && m_aiController) {
                m_aiController->humanMadeMove();
            }
        }
    }

    QMainWindow::mousePressEvent(event);
}

// 将鼠标位置转换为棋盘坐标
QPoint Client::convertToBoardPosition(const QPoint& mousePos)
{
    // 将鼠标位置从窗口坐标转换为棋盘视图坐标
    QPoint viewPos = ui->board->mapFromParent(mousePos);

    // 转换为场景坐标
    QPointF scenePos = ui->board->mapToScene(viewPos);

    // 检查是否在棋盘范围内
    if (scenePos.x() < m_boardStartX || scenePos.y() < m_boardStartY) {
        return QPoint(-1, -1);
    }

    // 修正坐标计算：减去起始位置后直接除以格子大小
    int x = static_cast<int>((scenePos.x() - m_boardStartX) / m_cellSize);
    int y = static_cast<int>((scenePos.y() - m_boardStartY) / m_cellSize);

    // 确保坐标在有效范围内 (0-14)
    if (x < 0 || x >= 15 || y < 0 || y >= 15) {
        return QPoint(-1, -1);
    }

    return QPoint(x, y);
}

bool Client::handleBoardClick(const QPoint& boardPos) {
    int x = boardPos.x();
    int y = boardPos.y();

    // 使用 Board 类检查位置是否为空
    if (m_board->getPiece(x, y) != PieceType::Empty) {
        qDebug() << "Position already occupied:" << x <<", "<< y;
        return false;
    }

    if (is_current_player == false && m_gameMode == PvP) {
        QMessageBox::information(this, "提示", "请等待对手落子");
        return false;
    }

    // 使用 Board 类处理落子逻辑
    PieceType pieceType = (m_currentPlayer == PieceType::Black) ? PieceType::Black : PieceType::White;

    // 落子时使用m_grid记录棋盘情况
    if (m_board->placePiece(x, y, pieceType)) {
        // 如果是网络对战模式，发送落子信息给服务器
        if (m_isConnected && m_gameMode == PvP) {
            std::cout<<"x = "<<x<<" y = "<<y<<std::endl;
            m_networkManager->sendMove(x, y, pieceType);
            // 表示轮到对方下棋了
            is_current_player = false;
        }
        if(is_winner){
            is_winner = false;
            // 发送重新开始游戏
            m_networkManager->sendResumeMessage(m_currentRoomId);
        }
        return true;
    }
}

// 处理棋子放置事件
void Client::onPiecePlaced(int x, int y, PieceType pieceType) {
    // 播放落子音效
    m_soundManager->playSound(SoundManager::PiecePlaced);
    // 绘制棋子
    drawPiece(x, y, pieceType);

    // 添加到历史记录
    m_moveHistory.append(QPoint(x, y));

    // 检查是否获胜（使用 Board 类的功能）
    if (m_board->checkWin(x, y, pieceType)) {
        is_current_player = true;
        is_winner = true;

        // 确定获胜者和失败者
        Player* winner = (pieceType == m_player1->getPieceType()) ? m_player1 : m_player2;
        Player* loser = (winner == m_player1) ? m_player2 : m_player1;

        loser->addLoss();      // 增加败场

        // 如果是网络对战，发送游戏结束和统计信息
        if (m_isConnected && m_gameMode == PvP && winner -> getName() == m_player1Name) {
            if (winner->getName() == m_player1Name) {
                m_player1->addScore(10);
                m_player1->addWin();
            }
            // 发送统计更新
            m_networkManager->sendStatsUpdate(m_player1Name,
                                              m_player1->getScore(),
                                              m_player1->getTotalWins(),
                                              m_player1->getTotalLosses(),
                                              m_player1->getTotalDraws(),
                                              m_player1->getStreak());
        }else if(m_gameMode == PvE && winner -> getName() == m_player1Name){
            m_player1->addScore(10);
            m_player1->addWin();
        }
        // 暂停计时器
        if (m_gameTimer->isActive()) {
            m_gameTimer->stop();
        }
        // 显示游戏结果对话框
        showGameResult(winner->getName() + " wins!");
        displayWinner(winner->getName());
    }
    // 检查是否平局
    else if (m_board->isFull()) {
        // 平局处理
        m_player1->addDraw();
        m_player2->addDraw();
        // m_gameState = GameOver;
        if (m_gameTimer->isActive()) {
            m_gameTimer->stop();
        }
        showGameResult("平局！");

        // 如果是网络对战，通知服务器游戏结束
        if (m_isConnected && m_gameMode == PvP) {
            m_networkManager->sendGameEnd("Draw");
        }
    }
    // 继续游戏
    else {
        // 切换玩家
        // m_currentPlayer = (m_currentPlayer == PieceType::Black) ? PieceType::White : PieceType::Black;
        // 更新UI显示当前玩家
        updatePlayerDisplay();
    }

    ui -> regret_btn->setEnabled(true);
}

void Client::updateCurrentPlayerDisplay()
{
    if (m_isConnected && m_gameMode == PvP) {
        if (is_current_player) {
            ui->statusBar->showMessage("轮到您落子");
        } else {
            ui->statusBar->showMessage("等待对手落子...");
        }
    }
}

void Client::updatePlayerDisplay()
{
    if (m_isConnected && m_gameMode == PvP) {
        // 更新状态栏提示
        if (is_current_player) {
            ui->statusBar->showMessage("轮到您落子");
        } else {
            ui->statusBar->showMessage("等待对手落子...");
        }
    }
}

// 处理棋盘清空事件
void Client::onBoardCleared() {
    // 清空场景并重新绘制棋盘
    gameScene->clear();
    drawChessBoard();

    // 重置游戏状态
    m_gameState = Playing;
}


// 绘制棋子
void Client::drawPiece(int x, int y, PieceType player)
{
    // 计算棋子在场景中的中心位置
    qreal centerX = m_boardStartX + x * m_cellSize;
    qreal centerY = m_boardStartY + y * m_cellSize;

    // 棋子半径（略小于格子的一半）
    qreal radius = m_cellSize * 0.4;

    // 根据玩家选择颜色
    QBrush brush;
    if (player == PieceType::Black) {
        brush = QBrush(Qt::black);
    } else {
        brush = QBrush(Qt::white);
    }

    // 创建棋子图形项
    QGraphicsEllipseItem* piece = gameScene->addEllipse(
        centerX - radius, centerY - radius,
        radius * 2, radius * 2,
        QPen(Qt::black, 1), brush);

    piece->setZValue(1);
}

// 检查是否获胜（需要实现）
bool Client::checkWin(int x, int y, PieceType player) {
    return m_board->checkWin(x, y, player);
}

// 检查棋盘是否已满
bool Client::isBoardFull() {
    return m_board->isFull();
}

void Client::recvClose(){
    this -> close();
}

void Client::on_regret_btn_clicked()
{
    if (m_moveHistory.isEmpty()) {
        return;
    }

    m_soundManager->playSound(SoundManager::Click);
    // 获取最后一步
    QPoint lastMove = m_moveHistory.last();
    m_moveHistory.removeLast();

    // 使用 Board 类清空该位置
    m_board->placePiece(lastMove.x(), lastMove.y(), PieceType::Empty);

    // 只移除最后一步的棋子（而不是重绘整个场景）
    removePieceFromScene(lastMove.x(), lastMove.y());

    // 只能悔棋一步
    ui->regret_btn->setEnabled(false);
}

// 从场景中移除指定位置的棋子
void Client::removePieceFromScene(int x, int y)
{
    m_board->setPos(x, y, PieceType::Empty);
    // 计算棋子在场景中的位置
    qreal centerX = m_boardStartX + x * m_cellSize;
    qreal centerY = m_boardStartY + y * m_cellSize;
    qreal radius = m_cellSize * 0.4;

    QRectF pieceRect(centerX - radius, centerY - radius, radius * 2, radius * 2);

    // 查找并移除该位置的棋子图形项
    QList<QGraphicsItem*> items = gameScene->items(pieceRect);
    for (QGraphicsItem* item : items) {
        // 该位置是否有棋子
        if (QGraphicsEllipseItem* ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
            // 找到那个棋子的位置，然后移除掉
            if (ellipse->rect() == pieceRect) {
                gameScene->removeItem(ellipse);
                delete ellipse;
                break;
            }
        }
    }
}


void Client::on_give_up_btn_clicked()
{
    // 检查游戏是否正在进行中
    if (m_gameState != Playing) {
        QMessageBox::information(this, "提示",
                                 QString("游戏%1，无法认输")
                                     .arg(m_gameState == Paused ? "尚未开始" : "已结束"));
        return;
    }

    // 检查是否是AI对战，如果是AI对战，可能需要特殊处理
    if (m_player2->isAI()) {
        QMessageBox::information(this, "提示", "AI玩家不能认输");
        return;
    }
    m_soundManager->playSound(SoundManager::Click);

    m_player2->addScore(10);
    m_player1->addLoss();
    m_player2->addWin();
    m_networkManager -> sendGiveUpMessage(m_player1->getName(), m_currentRoomId);
    showSurrenderDialog();
    m_networkManager->sendResumeMessage(m_currentRoomId);
}

void Client::RecvGiveUpInfo(const QString & username){
    std::cout<<"RecvGiveUpInfo username = "<<username.toStdString()<<std::endl;
    if(m_player2->getName() == username){
        m_player1->addScore(10);
        m_player1->addWin();
    }
    showGameResult(m_player1->getName() + " wins");
}

void Client::showSurrenderDialog()
{
    // 创建确认对话框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认认输");
    msgBox.setText("您确定要认输吗？");
    msgBox.setIcon(QMessageBox::Question);

    // 添加确认和取消按钮
    QPushButton* confirmButton = msgBox.addButton("确认认输", QMessageBox::YesRole);
    QPushButton* cancelButton = msgBox.addButton("取消", QMessageBox::NoRole);

    // 设置默认按钮为取消
    msgBox.setDefaultButton(cancelButton);

    // 显示对话框
    msgBox.exec();

    // 处理用户选择
    if (msgBox.clickedButton() == confirmButton) {
        handleSurrender();
    }
}

void Client::handleSurrender()
{
    // 设置游戏状态为结束
    // m_gameState = GameOver;

    // 暂停计时器
    if (m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 确定获胜者和失败者
    // 认输方是当前玩家，所以获胜方是对方
    Player* winner = (m_currentPlayer == m_player1->getPieceType()) ? m_player2 : m_player1;
    Player* loser = (m_currentPlayer == m_player1->getPieceType()) ? m_player1 : m_player2;

    // 更新分数和统计
    loser->addLoss();     // 增加败场

    // 更新UI显示
    updatePlayerStats();

    // 显示认输结果
    QString surrenderMessage = QString("%1 认输，%2 获胜！")
                                   .arg(loser->getName())
                                   .arg(winner->getName());

    // 显示游戏结果对话框（复用之前的showGameResultDialog）
    showGameResultDialog(surrenderMessage);

    qDebug() << loser->getName() << "认输，" << winner->getName() << "获胜";
}


void Client::on_peace_btn_clicked()
{
    // 检查游戏是否正在进行中
    if (m_gameState != Playing) {
        QMessageBox::information(this, "提示", "游戏尚未开始或已结束，无法求和");
        return;
    }

    // 检查是否是AI对战
    if (m_player2->isAI()) {
        QMessageBox::information(this, "提示", "不能向AI玩家求和");
        return;
    }

    handlePeaceRequest();

    // 发送统计更新
    m_networkManager->sendStatsUpdate(m_player1Name,
                                      m_player1->getScore(),
                                      m_player1->getTotalWins(),
                                      m_player1->getTotalLosses(),
                                      m_player1->getTotalDraws(),
                                      m_player1->getStreak());

    m_networkManager->sendPeaceMessage();
    m_networkManager->sendResumeMessage(m_currentRoomId);
}

void Client::recvPeace(){
    m_player1->addScore(5);
    m_player1->addDraw();
    showGameResult(m_player1->getName() + " and " + m_player2->getName() + " peace");
}

void Client::handlePeaceRequest()
{
    // 如果是PvE模式，直接处理AI的响应
    if (m_player2->isAI()) {
        // AI随机决定是否接受求和（50%概率）
        bool aiAccepts = QRandomGenerator::global()->bounded(2) == 0;
        processPeaceResponse(aiAccepts);
        return;
    }

    // PvP模式：显示求和请求对话框
    showPeaceRequestDialog();
}

void Client::showPeaceRequestDialog()
{
    // 创建求和确认对话框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("求和请求");
    msgBox.setText(QString("%1 请求和棋，是否同意？").arg(m_player2->getName()));
    msgBox.setIcon(QMessageBox::Question);

    // 添加同意和拒绝按钮
    QPushButton* acceptButton = msgBox.addButton("同意", QMessageBox::YesRole);
    QPushButton* rejectButton = msgBox.addButton("拒绝", QMessageBox::NoRole);

    // 设置默认按钮
    msgBox.setDefaultButton(rejectButton);

    // 显示对话框（在PvP模式下，这应该是给对手看的）
    // 注意：在实际网络对战中，这个请求应该发送给对手
    msgBox.exec();

    // 处理对手的响应
    bool accepted = (msgBox.clickedButton() == acceptButton);
    processPeaceResponse(accepted);
}

void Client::processPeaceResponse(bool accepted)
{
    if (accepted) {
        // 暂停计时器
        if (m_gameTimer->isActive()) {
            m_gameTimer->stop();
        }

        // 双方各加5分（平局奖励）
        m_player1->addScore(5);
        m_player2->addScore(5);

        // 更新统计（平局不计入胜负）
        m_player1->addDraw();
        m_player2->addDraw();

        // 更新UI显示
        updatePlayerStats();

        // 显示平局结果
        showGameResultDialog("游戏以和棋结束！");

        qDebug() << "和棋达成，双方各得5分";
    } else {
        // 求和被拒绝
        QMessageBox::information(this, "求和结果",
                                 QString("您的求和请求被%1拒绝").arg(m_player2->getName()));

        qDebug() << "求和被拒绝";
    }
}


void Client::on_pause_btn_clicked()
{
    if (isPause) {
        // 当前是暂停状态，点击后启动计时器
        m_gameTimer->start(1000);
        ui->pause_btn->setText("暂停");
        isPause = false;
    } else {
        // 当前是运行状态，点击后暂停计时器
        m_gameTimer->stop();
        ui->pause_btn->setText("继续");
        isPause = true;
    }
}


void Client::on_exit_game_btn_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认退出",
                                  "确定要退出当前游戏吗？对手会收到您退出的通知。",
                                  QMessageBox::Yes | QMessageBox::No);

    if (s_gameMode == "pvp" && reply == QMessageBox::Yes) {
        // 如果是网络对战，通知对手游戏结束
        if (m_isConnected && !m_currentOpponent.isEmpty()) {
            // 使用现有的发送游戏结束消息的方法
            m_networkManager->sendGameEnd(m_currentOpponent);
        }

        // 本地处理退出到游戏选择界面
        exitToMainMenu();
    }else if (s_gameMode == "pve" && reply == QMessageBox::Yes) {
        safelyExitAIGame();
    }
}

void Client::safelyExitAIGame()
{
    // 1. 先断开信号连接，避免循环调用
    disconnect(m_aiController, &AIGameController::exitToSelection,
               this, &Client::exitToSelection);

    // 2. 停止所有计时器
    if (m_gameTimer && m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 3. 重置游戏状态
    // m_gameState = NotStarted;

    // 4. 清空棋盘但不删除共享资源
    if (m_board) {
        m_board->clear();
    }
    gameScene->clear();
    drawChessBoard();

    // 5. 清空历史记录
    m_moveHistory.clear();

    // 6. 重置计时器显示
    elapsedTime = 0;
    ui->timeEdit->setTime(QTime(0, 0, 0));

    // 7. 更新UI状态
    ui->regret_btn->setEnabled(false);
    ui->pause_btn->setEnabled(false);

    // 8. 直接退出到主菜单，不通过AI控制器
    exitToMainMenu();
}

void Client::updateGameTimer() {
    elapsedTime++;
    QTime time(0, elapsedTime / 60, elapsedTime % 60);
    ui->timeEdit->setTime(time);
}

void Client::updatePlayerStats() {
    ui->score_one->setText(QString("积分: %1").arg(m_player1->getScore()));
    ui->score_two->setText(QString("积分: %1").arg(m_player2->getScore()));

    // 计算总游戏场次
    int totalGames = m_player1->getTotalWins() + m_player1->getTotalLosses();

    ui->win_number->setText(QString("胜场: %1").arg(m_player1->getTotalWins()));
    ui->win_ratio->setText(QString("胜率: %1%").arg(m_player1->getWinRate(totalGames), 0, 'f', 1));

    // 可以添加更多统计信息显示
    ui->win_number->setText(QString("连胜: %1").arg(m_player1->getStreak()));
    ui->level->setText(QString("等级: %1").arg(m_player1->getLevel()));

    ui->draw_number->setText(QString("平局: %1").arg(m_player1->getTotalDraws()));
}


void Client::showGameResult(const QString& result) {
    if (result.contains("wins") && result.contains(m_player1Name)) {
        m_soundManager->playSound(SoundManager::Win);
    } else if (result.contains("wins")) {
        m_soundManager->playSound(SoundManager::Lose);
    } else if (result.contains("平局") || result.contains("peace")) {
        m_soundManager->playSound(SoundManager::Draw);
    }

    showGameResultDialog(result);
}

void Client::showGameResultDialog(const QString& message)
{
    // 暂停游戏计时器
    if (m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 创建自定义消息框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("游戏结束");
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);

    // 添加三个按钮
    QPushButton* endGameButton = msgBox.addButton("结束游戏", QMessageBox::AcceptRole);
    QPushButton* continueButton = msgBox.addButton("继续新局", QMessageBox::ActionRole);
    QPushButton* quitAppButton = msgBox.addButton("退出程序", QMessageBox::RejectRole);

    // 设置默认按钮
    msgBox.setDefaultButton(continueButton);

    // 显示模态对话框
    msgBox.exec();

    // 处理按钮点击
    QAbstractButton* clickedButton = msgBox.clickedButton();
    if (clickedButton == endGameButton) {
        exitToMainMenu();
    } else if (clickedButton == continueButton) {
        restartGame();
    } else if (clickedButton == quitAppButton) {
        quitApplication();
    }
}

void Client::onGameResultButtonClicked(QAbstractButton* button) {
    QString buttonText = button->text();

    if (buttonText == "结束游戏") {
        exitToMainMenu();
    } else if (buttonText == "继续新局") {
        restartGame();
    } else if (buttonText == "退出程序") {
        quitApplication();
    }
}

void Client::restartGame() {
    // 重置游戏状态
    m_gameState = Playing;

    // 清空棋盘
    m_board->clear();
    m_gameBoard.clear();

    // 重置移动历史
    m_moveHistory.clear();


    // 重置计时器
    elapsedTime = 0;
    ui->timeEdit->setTime(QTime(0, 0, 0));

    // 启动计时器
    if (!m_gameTimer->isActive()) {
        m_gameTimer->start(1000);
    }

    // 更新UI显示
    updatePlayerStats();
    ui->regret_btn->setEnabled(false);

    if(s_gameMode == "pve"){
        m_aiController->startGame();
    }

    qDebug() << "开始新游戏";
}

void Client::recvResumStart(){
    this->m_gameTimer->start();
}

void Client::exitToMainMenu() {
    // 发送返回信号
    emit returnToSelection();

    // 关闭当前窗口
    this->close();
}

void Client::quitApplication() {
    // 退出整个应用程序
    QApplication::quit();
}


void Client::onChatMessageSent(const QString &message)
{
    if (m_networkManager && m_networkManager->isConnected()) {
        // 在本地聊天框中显示自己的消息
        // m_chatDialog->addMessage(m_player1Name, message);
        m_networkManager->sendChatMessage(m_player1Name, message);
    } else {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法发送消息");
    }
}


void Client::onMoveReceived(int x, int y, PieceType piece)
{
    std::cout<<"current piece type = "<<(int)m_currentPlayer<<std::endl;
    std::cout<<"onMoveReceived(x = "<<x<<", y = "<<y<<")"<<" piece = "<<(int)piece<<std::endl;
    std::cout<<"m_player2->getPieceType = "<<(int)(m_player2->getPieceType())<<std::endl;
    // 验证棋子颜色是否正确
    if (piece != m_player2->getPieceType()) {
        qWarning() << "Received move with wrong piece type";
        return;
    }
    // 处理接收到的走棋消息
    if (m_board->placePiece(x, y, piece)) {
        m_moveHistory.append(QPoint(x, y));

        // 切换当前玩家
        is_current_player = true;
        updatePlayerDisplay();

        // 检查游戏状态
        if (m_board->checkWin(x, y, piece)) {
            m_board->clear();
            is_current_player = true;
            // 确定获胜者和失败者
            Player* winner = (piece == m_player1->getPieceType()) ? m_player1 : m_player2;
            Player* loser = (winner == m_player1) ? m_player2 : m_player1;

            // if (winner->getName() == m_currentOpponent) {
            //     m_player2->addScore(10);
            //     m_player2->addWin();
            // }

            loser->addLoss();     // 增加败场

            // 暂停计时器
            if (m_gameTimer->isActive()) {
                m_gameTimer->stop();
            }

            displayWinner(winner->getName());
            // 显示游戏结果对话框
            showGameResult(winner->getName() + " wins!");
        }
    }
}

void Client::onGameStartReceived(const QString &opponent, PieceType myPieceType, PieceType opponentPieceType)
{
    // 重置游戏状态
    initializeGame();

    // 启动定时器
    this->m_gameTimer->start();

    qDebug() << "=== 游戏开始调试 ===";
    qDebug() << "我的用户名:" << m_player1Name;
    qDebug() << "我的棋子:" << (myPieceType == PieceType::Black ? "黑棋" : "白棋");
    qDebug() << "对手用户名:" << opponent;
    qDebug() << "对手棋子:" << (opponentPieceType == PieceType::Black ? "黑棋" : "白棋");

    // 固定设置玩家身份（游戏开始后不再更改）
    m_player1->setName(m_player1Name);  // 固定自己的名字
    m_player1->setPieceType(myPieceType); // 固定自己的棋子类型

    m_player2Name = opponent;
    m_player2->setName(opponent);       // 固定对手的名字
    m_player2->setPieceType(opponentPieceType); // 固定对手的棋子类型

    m_currentOpponent = opponent;       // 固定当前对手

    // 设置当前玩家状态
    ui->player_one->setText(QString("当前玩家: %1 (%2)")
                                .arg(m_player1->getName())
                                .arg(m_player1->getPieceType() == PieceType::Black ? "黑棋" : "白棋"));
    ui->player_two->setText(QString("对战玩家: %1 (%2)")
                                .arg(m_player2->getName())
                                .arg(m_player2->getPieceType() == PieceType::Black ? "黑棋" : "白棋"));

    // 设置当前玩家（黑棋先手）
    m_currentPlayer = myPieceType;

    is_current_player = true;

    // 更新显示（使用固定信息）
    updatePlayerDisplay();

    m_chatDialog->addMessage("系统", QString("游戏开始！您执%1 vs %2 执%3")
                                         .arg(myPieceType == PieceType::Black ? "黑棋" : "白棋")
                                         .arg(opponent)
                                         .arg(opponentPieceType == PieceType::Black ? "黑棋" : "白棋"));
}

void Client::onOpponentExited()
{
    // 停止计时器
    if (m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 更新游戏状态
    // m_gameState = GameOver;

    // 显示提示信息
    QMessageBox::information(this, "对手退出",
                             "您的对手已经退出游戏，游戏结束");

    // 在聊天框中通知
    m_chatDialog->addMessage("系统", "对手已退出游戏");

    // 更新UI
    ui->statusBar->showMessage("对手退出，游戏结束");
    ui->create_room->setEnabled(true);
    ui->join_room->setEnabled(true);

    // 清空对手信息
    m_currentOpponent.clear();
    ui->player_two->setText("对战玩家: 等待对手...");
}

void Client::onGameEndReceived(const QString &winner)
{
    // 检查是否是对方退出游戏的通知
    if (winner == m_currentOpponent) {
        // 对手退出游戏
        onOpponentExited();
        return;
    }

    // 停止计时器
    if (m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 更新UI显示
    ui->statusBar->showMessage("游戏结束");

    // 启用房间操作按钮
    ui->create_room->setEnabled(true);
    ui->join_room->setEnabled(true);

    // 更新玩家统计
    updatePlayerStats();

    m_chatDialog->addMessage("系统", "=== 游戏结束 ===");

    // 重置游戏状态，准备新游戏
    resetGameAfterEnd();
}

void Client::displayWinner(const QString& winner){
    // 处理胜负结果
    QString resultMessage;
    if (winner == m_player1Name) {
        resultMessage = "恭喜！您获得了胜利！";
        m_chatDialog->addMessage("系统", "🎉🎉 您获得了胜利！");
    } else if (winner == m_currentOpponent) {
        resultMessage = m_currentOpponent + " 获得了胜利";
        m_chatDialog->addMessage("系统", m_currentOpponent + " 获得了胜利");
    } else if (winner == "Draw") {
        resultMessage = "游戏平局！";
        m_chatDialog->addMessage("系统", "游戏以平局结束");
    } else {
        resultMessage = winner + " 获得了胜利";
        m_chatDialog->addMessage("系统", winner + " 获得了胜利");
    }
}

void Client::resetGameAfterEnd()
{
    // 清空棋盘但保持界面状态
    m_board->clear();
    m_gameBoard.clear();
    m_moveHistory.clear();
    gameScene->clear();
    drawChessBoard();

    // 重置计时器显示但不启动
    elapsedTime = 0;
    ui->timeEdit->setTime(QTime(0, 0, 0));

    updatePlayerDisplay();

    // 禁用悔棋按钮
    ui->regret_btn->setEnabled(false);

    // 清空对手信息
    m_currentOpponent.clear();
    ui->player_two->setText("对战玩家: 等待对手...");
}

void Client::onChatMessageReceived(const QString& sender, const QString &message)
{
    if (sender != m_player1Name) {
        m_soundManager->playSound(SoundManager::Notification);
    }
    // 格式化聊天消息
    QString formattedMessage;

    if (sender == m_player1Name) {
        // 自己发送的消息（可能来自其他客户端）
        formattedMessage = "[我] " + message;
    } else if (sender == m_player2Name && !m_player2Name.isEmpty()) {
        // 当前对手的消息
        formattedMessage = "[对手] " + message;
    } else {
        // 其他玩家的消息
        formattedMessage = "[" + sender + "] " + message;
    }

    // 在聊天框中显示消息
    // appendToChatFrame(formattedMessage);
    m_chatDialog->addMessage(sender, formattedMessage);

    // 播放消息提示音或显示通知
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
    if (sender != m_player1Name && !isActiveWindow()) {
        // 如果窗口不是活动窗口，显示系统通知
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon::Information;
        QString title = "新消息来自 " + sender;
        trayIcon->showMessage(title, message, icon, 3000);
    }
}

void Client::onNetworkConnected()
{
    m_soundManager->playSound(SoundManager::Notification);

    m_led->setStatus(LedIndicator::BlinkingGreen);
    m_isConnected = true;

    // 连接成功后发送登录信息给服务端（实际来说应该登录的那一刻发生，但是感觉有点麻烦了，就统一放在这里处理了）
    m_networkManager->login(m_player1Name);

    ui->statusBar->showMessage("已连接到服务器");

    // 更新UI状态
    ui->conn_dialog->setText("断开连接");
    ui->conn_dialog->setStyleSheet("background-color: #4CAF50; color: white;");

    // 启用网络相关功能
    enableNetworkFeatures(true);

    QMessageBox::information(this, "连接成功", "成功连接到服务器");
}

void Client::onNetworkDisconnected()
{
    m_led->setStatus(LedIndicator::BlinkingRed);
    m_isConnected = false;

    ui->statusBar->showMessage("与服务器断开连接");

    // 如果正在游戏中，强制结束游戏
    if (m_gameState == Playing && !m_currentOpponent.isEmpty()) {
        // m_gameState = GameOver;
        if (m_gameTimer->isActive()) {
            m_gameTimer->stop();
        }
        QMessageBox::warning(this, "连接中断", "与服务器的连接已中断，游戏强制结束");
        resetGameAfterEnd();
    }

    // 清空在线用户列表
    clearOnlineUsers();

    // 更新UI状态
    ui->conn_dialog->setText("连接服务器");
    ui->conn_dialog->setStyleSheet("");

    // 禁用网络相关功能
    enableNetworkFeatures(false);

    QMessageBox::information(this, "断开连接", "已从服务器断开");

    // 如果是网络对战，通知服务器游戏结束
    if (m_isConnected && m_gameMode == PvP) {
        m_networkManager->sendGameEnd(m_player1Name);
    }
}

void Client::onNetworkError(const QString &error)
{
    m_soundManager->playSound(SoundManager::Error);

    ui->statusBar->showMessage("网络错误: " + error);
    QMessageBox::warning(this, "网络错误", error);
}

void Client::enableNetworkFeatures(bool enabled)
{
    // 如果是断开连接，也重置连接按钮
    if (!enabled) {
        ui->conn_dialog->setText("连接服务器");
        ui->conn_dialog->setStyleSheet("");
    }
}

QString Client::getLocalIp()
{
    QString hostName = QHostInfo::localHostName();

    QStringList availableIps;
    QString preferredIp;

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &interfac, interfaces) {
        // 跳过回环和未启用的接口
        if (interfac.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !interfac.flags().testFlag(QNetworkInterface::IsUp) ||
            !interfac.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }

        // ui->chat_frame->appendPlainText("📡 网络接口: " + interface.humanReadableName());

        // 获取该接口的所有IP地址
        QList<QNetworkAddressEntry> entries = interfac.addressEntries();
        foreach (const QNetworkAddressEntry &entry, entries) {
            QHostAddress ip = entry.ip();
            // 判断当前IP协议是否为IPv4
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ipStr = ip.toString();
                availableIps.append(ipStr);

                QString displayText;
                if (ip.isLoopback()) {
                    displayText = "➰ 回环: " + ipStr;
                } else if (ip.isInSubnet(QHostAddress("192.168.0.0"), 16)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("10.0.0.0"), 8)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("169.254.0.0"), 16)) {
                    displayText = "🔗 链路本地: " + ipStr;
                } else {
                    displayText = "🌍 公网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                }

                // 显示子网掩码
                displayText += " / " + entry.netmask().toString();
                // ui->chat_frame->appendPlainText("   " + displayText);
            }
        }
        // ui->chat_frame->appendPlainText("");
    }

    if (availableIps.isEmpty()) {
        return "127.0.0.1";
    }

    return preferredIp;
}

// 辅助函数：获取连接状态字符串
QString Client::getSocketStateString(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState: return "❌ 未连接";
    case QAbstractSocket::HostLookupState: return "🔍 正在查找主机...";
    case QAbstractSocket::ConnectingState: return "🔄 正在连接...";
    case QAbstractSocket::ConnectedState: return "✅ 已连接";
    case QAbstractSocket::ClosingState: return "⏹️ 正在关闭...";
    default: return "❓ 未知状态";
    }
}


void Client::on_conn_dialog_clicked()
{
    if(ui->conn_dialog->text() == "断开连接"){
        m_isConnected = false;
        m_currentRoomId.clear();
        m_currentRoomName.clear();
        m_networkManager->disconnectFromServer();
        return;
    }
    m_led->setStatus(LedIndicator::BlinkingYellow);
    if (m_isConnected) {
        // 如果已连接，点击时断开连接
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认断开",
                                      "确定要断开与服务器的连接吗？",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_networkManager->disconnectFromServer();
            m_isConnected = false;
            enableNetworkFeatures(false);
            ui->statusBar->showMessage("已断开连接");
        }
    } else {
        // 如果未连接，显示连接对话框
        QUrl serverUrl = ConnectionDialog::getConnectionInfo(this);

        if (serverUrl.isEmpty()) {
            return;
        }

        if (!serverUrl.isValid()) {
            QMessageBox::warning(this, "错误", "无效的服务器地址");
            return;
        }

        ui->statusBar->showMessage("正在连接服务器...");

        if (m_networkManager->connectToServer(serverUrl)) {
            // 连接中状态
            ui->conn_dialog->setText("连接中...");
        } else {
            ui->statusBar->showMessage("连接失败");
            QMessageBox::warning(this, "连接错误", "无法连接到服务器");
        }
    }
}
void Client::onLoginSuccess(const QString &username)
{
    ui->statusBar->showMessage("登录成功: " + username);
    QMessageBox::information(this, "登录成功", "欢迎 " + username);

    // // 更新UI状态
    m_isLoggedIn = true;
    // 延迟1秒后请求用户列表，确保登录处理完成
    QTimer::singleShot(1000, this, [this]() {
        if (m_networkManager && m_networkManager->isConnected()) {
            m_networkManager->requestUserList();
        }
    });
}

void Client::onLoginFailed(const QString &error)
{
    ui->statusBar->showMessage("登录失败: " + error);
    QMessageBox::warning(this, "登录失败", error);

    m_isLoggedIn = false;
    // updateLoginStatus();
}

void Client::onLogoutReceived(const QString &username)
{
    ui->statusBar->showMessage("用户退出: " + username);
    m_chatDialog->addMessage(username, " 离开了游戏");
}

// 添加处理用户列表的函数
void Client::onUserListReceived(const QStringList &users)
{
    m_members = users;
    clearOnlineUsers();
    for (const QString &user : users) {
        addOnlineUser(user);
    }
}

void Client::onUserStatusChanged(const QString &username, const QString &action)
{
    if (action == "joined") {
        addOnlineUser(username);
        m_chatDialog->addMessage(username , " 加入了游戏");
        updateOnlineUsersList();
    } else if (action == "left") {
        removeOnlineUser(username);
        m_chatDialog->addMessage(username , " 离开了游戏");
        updateOnlineUsersList();
    }
}


void Client::showOnlineUsersDialog()
{
    if(m_isConnected){

    }
    m_onlineUsersManager->showDialog();
}

void Client::onUserDoubleClicked(const QString &username)
{
    // 处理用户双击事件，例如开始私聊
    m_chatDialog->addMessage("系统", QString("您双击了用户: %1").arg(username));
}

void Client::onRefreshUsersRequested()
{
    // 处理刷新请求，例如重新从服务器获取用户列表
    if (m_networkManager && m_networkManager->isConnected()) {
        m_networkManager->requestRoomList();
    }

    for(int i = 0; i < m_members.size(); i++){
        addOnlineUser(m_members[i]);
    }
}

// 修改原有的用户管理函数
void Client::addOnlineUser(const QString &username)
{
    m_onlineUsersManager->addUser(username);
    m_chatDialog->addMessage("系统", username + " 加入了游戏");
}

void Client::removeOnlineUser(const QString &username)
{
    m_onlineUsersManager->removeUser(username);
    m_chatDialog->addMessage("系统", username + " 离开了游戏");
}

void Client::clearOnlineUsers()
{
    m_onlineUsersManager->clearUsers();
}

void Client::updateOnlineUsersList()
{
    // 现在这个函数可以简化或删除，因为状态栏更新通过信号槽自动处理
    if (statusBar()) {
        statusBar()->showMessage(
            QString("在线用户: %1 人").arg(m_onlineUsersManager->getUserCount()));
    }
}

void Client::updateStatusBar()
{
    QString statusText;

    if (m_isLoggedIn) {
        statusText = "已登录 - 用户: " + m_player1Name;
        if (m_networkManager->isConnected()) {
            statusText += " - 服务器: 已连接";
        } else {
            statusText += " - 服务器: 断开";
        }
    } else {
        statusText = "未登录";
        if (m_networkManager->isConnected()) {
            statusText += " - 服务器: 已连接";
        } else {
            statusText += " - 服务器: 断开";
        }
    }

    statusText += " - 在线用户: " + QString::number(m_onlineUsers.size());

    ui->statusBar->showMessage(statusText);
}

void Client::onChatActionTriggered()
{
    if (m_chatDialog->isHidden()) {
        m_chatDialog->show();
        m_chatDialog->raise();
        m_chatDialog->activateWindow();
    } else {
        m_chatDialog->hide();
    }
}

void Client::onCreateRoom()
{
    if (!m_networkManager || !m_networkManager->isConnected()) {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法创建房间");
        return;
    }

    // 创建房间对话框
    QDialog createRoomDialog(this);
    createRoomDialog.setWindowTitle("创建房间");
    createRoomDialog.setMinimumWidth(300);

    QFormLayout layout(&createRoomDialog);

    QLineEdit roomNameEdit;
    QLineEdit roomIdEdit;

    layout.addRow("房间名称:", &roomNameEdit);
    layout.addRow("房间ID:", &roomIdEdit);

    // 按钮区域
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &createRoomDialog);
    layout.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &createRoomDialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &createRoomDialog, &QDialog::reject);

    if (createRoomDialog.exec() == QDialog::Accepted) {
        QString roomName = roomNameEdit.text().trimmed();
        QString roomId = roomIdEdit.text().trimmed();

        if (roomName.isEmpty()) {
            QMessageBox::warning(this, "错误", "房间名称不能为空");
            return;
        }

        if (roomId.isEmpty()) {
            QMessageBox::warning(this, "错误", "房间ID不能为空");
            return;
        }

        // 检查房间ID格式（可选：字母数字组合）
        QRegularExpression regex(".*[a-zA-Z0-9].*");  // 添加 ^ 表示字符串开始
        QRegularExpressionMatch match = regex.match(roomId);
        if (!match.hasMatch()) {
            QMessageBox::warning(this, "错误", "房间ID格式不正确");
            return;
        }

        // 发送创建房间请求
        m_networkManager->createRoom(roomName, roomId);

        QMessageBox::information(this, "提示", "房间创建请求已发送，等待服务器响应...");
    }
}

void Client::onJoinRoom()
{
    if (!m_networkManager || !m_networkManager->isConnected()) {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法加入房间");
        return;
    }

    bool ok;
    QString roomId = QInputDialog::getText(this,
                                           "加入房间",
                                           "请输入要加入的房间ID:",
                                           QLineEdit::Normal,
                                           "",
                                           &ok);

    if (ok && !roomId.isEmpty()) {
        roomId = roomId.trimmed();

        // 检查房间ID格式
        QRegularExpression regex(".*[a-zA-Z0-9].*");  // 添加 ^ 表示字符串开始
        QRegularExpressionMatch match = regex.match(roomId);
        if (!match.hasMatch()) {
            QMessageBox::warning(this, "错误", "房间ID只能包含字母、数字、下划线和连字符");
            return;
        }

        // 发送加入房间请求
        m_networkManager->joinRoom(roomId);

        QMessageBox::information(this, "提示", "加入房间请求已发送，等待服务器响应...");
    }
}

void Client::onRoomCreated(const QString &roomId, const QString &roomName)
{
    m_currentRoomId = roomId;
    m_currentRoomName = roomName;

    QString message = QString("房间创建成功！\n房间名称: %1\n房间ID: %2")
                          .arg(roomName)
                          .arg(roomId);

    QMessageBox::information(this, "成功", message);
    ui->statusBar->showMessage("当前房间: " + roomName);

    // 更新UI状态
    ui->create_room->setEnabled(false); // 不能再创建新房间
    ui->join_room->setEnabled(false);   // 不能再加入其他房间
}

void Client::onRoomJoined(const QString &roomId, const QString &roomName, const QStringList &members)
{
    m_currentRoomId = roomId;
    m_currentRoomName = roomName;

    m_members = members;

    QString message = QString("成功加入房间！\n房间名称: %1\n房间ID: %2\n成员数量: %3人")
                          .arg(roomName)
                          .arg(roomId)
                          .arg(members.size());

    QMessageBox::information(this, "成功", message);
    ui->statusBar->showMessage("当前房间: " + roomName);

    // 更新UI状态
    ui->create_room->setEnabled(false); // 不能再创建新房间
    ui->join_room->setEnabled(false);   // 不能再加入其他房间

    // 显示对战网友
    for(int i = 0; i < members.size(); i++){
        addOnlineUser(members[i]);
    }
}

void Client::onRoomError(const QString &errorMessage)
{
    QMessageBox::warning(this, "房间操作失败", errorMessage);
}

void Client::onRoomLeft(const QString &roomId)
{
    if (roomId.isEmpty()) {
        QMessageBox::information(this, "提示", "您当前不在任何房间中");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认离开",
                                  "确定要离开当前房间吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_networkManager->leaveRoom(roomId);

        // 立即更新本地状态
        m_currentRoomId.clear();
        m_currentRoomName.clear();
        ui->statusBar->showMessage("未加入房间");

        // 恢复UI状态
        ui->create_room->setEnabled(true);
        ui->join_room->setEnabled(true);

        QMessageBox::information(this, "成功", "已离开房间");
    }
}



void Client::onRoomListReceived(const QJsonArray &rooms)
{
    m_roomListWidget->clear();

    if (rooms.isEmpty()) {
        m_roomListWidget->addItem("暂无房间");
        return;
    }

    for (const QJsonValue &roomValue : rooms) {
        QJsonObject room = roomValue.toObject();

        QString roomId = room["roomId"].toString();
        QString roomName = room["roomName"].toString();
        QString creator = room["creator"].toString();
        int memberCount = room["memberCount"].toInt();
        QString createTime = room["createTime"].toString();

        QString displayText = QString("%1 (ID: %2)\n创建者: %3 | 成员: %4人 | 创建时间: %5")
                                  .arg(roomName)
                                  .arg(roomId)
                                  .arg(creator)
                                  .arg(memberCount)
                                  .arg(createTime);

        QListWidgetItem *item = new QListWidgetItem(displayText, m_roomListWidget);
        item->setData(Qt::UserRole, roomId); // 存储房间ID
        item->setToolTip(displayText);

        // 根据成员数量设置不同的背景色
        if (memberCount >= 2) {
            item->setBackground(QColor(255, 200, 200)); // 房间已满或接近满员
        } else if (memberCount == 1) {
            item->setBackground(QColor(200, 255, 200)); // 房间有空位
        }
    }
}

void Client::onStatsUpdated(const QString &username, int score,
                            int wins, int losses, int draws, int streak) {
    std::cout<<"onStatsUpdated"<<" username = "<<username.toStdString()<<std::endl;
    if (username == m_player2Name) {
        // 更新对手的统计信息
        m_player2->setScore(score);
        // 注意：需要在 Player 类中添加 setter 方法
        m_player2->setWins(wins);
        m_player2->setLosses(losses);
        m_player2->setDraws(draws);
        m_player2->setStreak(streak);

        // 更新UI显示
        updatePlayerStats();
    }
}

void Client::onSoundSettingsTriggered()
{
    QDialog dialog(this);
    dialog.setWindowTitle("音效设置");

    QVBoxLayout layout(&dialog);

    QSlider *volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(m_soundManager->volume());

    QCheckBox *muteCheckbox = new QCheckBox("静音");
    muteCheckbox->setChecked(m_soundManager->isMuted());

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout.addWidget(new QLabel("音量:"));
    layout.addWidget(volumeSlider);
    layout.addWidget(muteCheckbox);
    layout.addWidget(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_soundManager->setVolume(volumeSlider->value());
        m_soundManager->setMuted(muteCheckbox->isChecked());
    }
}
