#include "AIGameController.h"
#include "client.h"
#include "ui_client.h"
#include <QMessageBox>
#include <QTime>
#include <QDebug>
#include <algorithm>
#include <random>

AIGameController::AIGameController(Client* client, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_board(nullptr)  // 使用共享的棋盘
    , m_humanPlayer(nullptr)
    , m_aiPlayer(nullptr)
    , m_gameTimer(new QTimer(this))
    , m_gameState(NotStarted)
    , m_elapsedTime(0)
    , m_isAITurn(false)
    , m_currentPlayer(PieceType::Empty)
{
    // 连接计时器信号
    connect(m_gameTimer, &QTimer::timeout, this, &AIGameController::updateGameTimer);
}

AIGameController::~AIGameController()
{
    delete m_board;
    delete m_humanPlayer;
    delete m_aiPlayer;
}

void AIGameController::initializeGame()
{
    // 清理旧资源
    delete m_board;
    delete m_humanPlayer;
    delete m_aiPlayer;

    m_board = new Board(15);
    m_board->clear();  // 直接清空共享棋盘

    // 创建玩家 - AI执白棋先手，人类执黑棋后手
    m_aiPlayer = new Player("AI", PieceType::Black, true);
    m_humanPlayer = new Player(m_client->getPlayer2()->getName(), PieceType::White, false);

    // 重置游戏状态
    m_gameState = Playing;
    m_currentPlayer = PieceType::White; // AI先手，并且是黑棋
    m_isAITurn = true;
    m_elapsedTime = 0;

    // 清空棋盘
    m_board->clear();

    // 更新当前状态栏的显示
    if (m_client) {
        m_client->updatePlayerDisplay();
        m_client->getUI()->timeEdit->setTime(QTime(0, 0, 0));
    }

    // AI先手，开始思考
    QTimer::singleShot(1000, this, &AIGameController::aiMakeMove);
}

// 从客户端同步棋盘状态到AI
void AIGameController::syncBoardFromClient()
{
    if (!m_client || !m_board) return;

    // 获取客户端的棋盘状态并复制到AI的棋盘
    Board* clientBoard = m_client->getBoard();
    if (clientBoard) {
        m_board->copyFrom(*clientBoard);
    }
}

void AIGameController::syncPlayerFromClient(){
    Player* clientPlayer1 = m_client->getPlayer1();
    Player* clientPlayer2 = m_client->getPlayer2();
    if(clientPlayer1){
        m_humanPlayer->copyFrom(*clientPlayer1);
    }
    if(clientPlayer2){
        m_aiPlayer->copyFrom(*clientPlayer2);
    }
}


// 从AI同步棋盘状态到客户端
void AIGameController::syncBoardToClient()
{
    if (!m_client || !m_board) return;

    // 获取AI的棋盘状态并复制到客户端的棋盘
    Board* clientBoard = m_client->getBoard();
    if (clientBoard) {
        clientBoard->copyFrom(*m_board);
    }
}

void AIGameController::syncPlayerToClient(){
    Player* clientPlayer1 = m_client->getPlayer1();
    Player* clientPlayer2 = m_client->getPlayer2();
    if(clientPlayer1){
        clientPlayer1->copyFrom(*m_humanPlayer);
    }
    if(clientPlayer2){
        clientPlayer2->copyFrom(*m_aiPlayer);
    }
}


void AIGameController::startGame()
{
    initializeGame();

    // 启动计时器
    m_gameTimer->start(1000);

    // 更新UI状态
    if (m_client) {
        // m_client->getUI()->startButton->setEnabled(false);
        // m_client->getUI()->exitButton->setEnabled(true);
        m_client->getUI()->statusBar->showMessage("游戏开始！AI先手");
    }
}

void AIGameController::exitGame()
{
    // 检查计时器是否有效
    if (m_gameTimer && m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }

    // 重置状态但不删除共享资源
    m_gameState = NotStarted;
    m_isAITurn = false;

    // 发出退出信号
    emit exitToSelection();
}


void AIGameController::updateGameTimer()
{
    m_elapsedTime++;
    QTime time(0, m_elapsedTime / 60, m_elapsedTime % 60);

    if (m_client) {
        m_client->getUI()->timeEdit->setTime(time);
    }
}

void AIGameController::aiMakeMove()
{
    if (m_gameState != Playing || !m_isAITurn || !m_board || !m_aiPlayer) {
        return;
    }

    try {
        // 决策前同步：确保AI看到最新的棋盘状态
        syncBoardFromClient();
        syncPlayerFromClient();

        // 寻找最佳落子位置
        QPoint bestMove = findBestMove();
        int x = bestMove.x();
        int y = bestMove.y();

        // 添加位置检查
        if (m_board->getPiece(x, y) != PieceType::Empty) {
            qWarning() << "AI试图在已有棋子的位置落子:" << x << y;
            return;
        }

        // 放置棋子
        if (m_board->placePiece(x, y, m_currentPlayer)) {
            // 同步到客户端棋盘
            syncBoardToClient();

            // 发送AI下棋信号
            emit aiMadeMove(x, y);

            // 绘制棋子
            if (m_client) {
                m_client->drawPiece(x, y, m_currentPlayer);
            }

            // 检查游戏状态（使用客户端的棋盘检查）
            Board* clientBoard = m_client->getBoard();
            if (clientBoard && clientBoard->checkWin(x, y, m_currentPlayer)) {
                // AI获胜
                m_gameState = GameOver;
                m_gameTimer->stop();

                if (m_aiPlayer && m_humanPlayer) {
                    std::cout<<"AI winner"<<std::endl;
                    m_aiPlayer->addWin();
                    m_aiPlayer->addScore(10);
                    m_humanPlayer->addLoss();
                    m_client->updatePlayerDisplay();
                }
                syncPlayerToClient();
                m_client->showGameResult("AI获胜！");
            }  else if (m_board->isFull()) {
                // 平局
                m_gameState = GameOver;
                if (m_gameTimer->isActive()) {
                    m_gameTimer->stop();
                }

                if (m_client) {
                    m_client->showGameResult("平局！");
                }
            } else {
                m_isAITurn = false;

                if (m_client) {
                    m_client->getUI()->statusBar->showMessage("轮到您落子");
                }
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "AI move error:" << e.what();
        // 出错时随机选择一个位置
        // ... 可以添加备用策略
    }
}

QPoint AIGameController::findBestMove() const
{
    // 首先检查AI是否能立即获胜
    for (int x = 0; x < 15; ++x) {
        for (int y = 0; y < 15; ++y) {
            // 如果当前是空位置的话，判断AI落子是否可以获胜
            if (m_board->getPiece(x, y) == PieceType::Empty) {
                // 模拟在此位置落子
                if (m_board->checkWin(x, y, m_currentPlayer)) {
                    return QPoint(x, y); // 直接获胜的位置
                }
            }
        }
    }

    // 检查是否需要防守（阻止玩家获胜）
    PieceType humanPiece = (m_currentPlayer == PieceType::Black)
                               ? PieceType::White : PieceType::Black;

    for (int x = 0; x < 15; ++x) {
        for (int y = 0; y < 15; ++y) {
            if (m_board->getPiece(x, y) == PieceType::Empty) {
                // 模拟玩家在此位置落子
                if (m_board->checkWin(x, y, humanPiece)) {
                    return QPoint(x, y); // 必须防守的位置
                }
            }
        }
    }

    // 评估函数：寻找最佳位置
    int bestScore = -10000;
    QPoint bestMove(7, 7); // 默认选择中心位置

    // 简单的评估策略：优先选择中心区域和已有棋子周围
    for (int x = 0; x < 15; ++x) {
        for (int y = 0; y < 15; ++y) {
            if (m_board->getPiece(x, y) == PieceType::Empty) {
                // 根据连续相同棋子的数量来进行评估，连续相同棋子的数量越多，分数越大，越需要关注
                int score = evaluatePosition(x, y, m_currentPlayer);

                // 中心区域加分
                int centerDist = std::abs(x - 7) + std::abs(y - 7);
                score += (14 - centerDist) * 2;

                // 保留最大分数的那个位置作为最终落子的位置
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = QPoint(x, y);
                }
            }
        }
    }

    return bestMove;
}

int AIGameController::evaluatePosition(int x, int y, PieceType piece) const
{
    int score = 0;

    // 检查四个方向
    int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int i = 0; i < 4; ++i) {
        int dx = directions[i][0];
        int dy = directions[i][1];

        // 正向检查
        int count = 0;
        for (int step = 1; step < 5; ++step) {
            int nx = x + step * dx;
            int ny = y + step * dy;

            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) break;
            if (m_board->getPiece(nx, ny) == piece) count++;
            else break;
        }

        // 反向检查
        for (int step = 1; step < 5; ++step) {
            int nx = x - step * dx;
            int ny = y - step * dy;

            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) break;
            if (m_board->getPiece(nx, ny) == piece) count++;
            else break;
        }

        // 根据连子数评分
        if (count >= 4) score += 1000;      // 四子
        else if (count == 3) score += 100;  // 三子
        else if (count == 2) score += 10;   // 两子
        else if (count == 1) score += 1;    // 单子
    }

    return score;
}

void AIGameController::humanMadeMove()
{
    // 人类下棋后，同步棋盘状态到AI
    syncBoardFromClient();

    m_isAITurn = true;
    // 延迟一段时间后让AI思考
    QTimer::singleShot(1000, this, &AIGameController::aiMakeMove);
}
