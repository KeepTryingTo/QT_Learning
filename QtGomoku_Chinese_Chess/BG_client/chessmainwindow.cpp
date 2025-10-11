// chessmainwindow.cpp
#include "chessmainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>

ChessMainWindow::ChessMainWindow(const QString& playerName, QWidget *parent)
    : QDialog(parent)
    , m_playerName(playerName)
    , m_humanColor(ChessPieceColor::Red) // 默认人类玩家为红方
{
    setWindowTitle("中国象棋 - " + playerName);
    setMinimumSize(800, 700);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();

    // 连接信号和槽
    connect(m_boardWidget, &ChessBoardWidget::gameStarted,this, &ChessMainWindow::onGameStarted);
    connect(m_boardWidget, &ChessBoardWidget::gameEnded,this, &ChessMainWindow::onGameEnded);
    connect(m_newGameBtn, &QPushButton::clicked,this, &ChessMainWindow::onNewGame);
    connect(m_exitBtn, &QPushButton::clicked,this, &ChessMainWindow::onExit);

    // 初始化游戏
    onNewGame();
}

void ChessMainWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // 标题标签
    QLabel *titleLabel = new QLabel("中国象棋-人机对战", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    // 状态标签
    m_statusLabel = new QLabel("当前玩家：红方 | 您是：红方 | 状态：进行中", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    QFont statusFont;
    statusFont.setPointSize(12);
    m_statusLabel->setFont(statusFont);

    // 棋盘部件 - 增加拉伸因子，让棋盘占用更多空间
    m_boardWidget = new ChessBoardWidget(this);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_newGameBtn = new QPushButton("新游戏", this);
    m_exitBtn = new QPushButton("退出", this);

    // 设置按钮样式
    m_newGameBtn->setMinimumSize(100, 40);
    m_exitBtn->setMinimumSize(100, 40);
    m_newGameBtn->setEnabled(false); // 新游戏按钮初始为灰色

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_newGameBtn);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_exitBtn);
    buttonLayout->addStretch();

    // 调整布局比例：棋盘占用更多空间
    mainLayout->addWidget(titleLabel, 0);        // 不拉伸
    mainLayout->addWidget(m_statusLabel, 0);    // 不拉伸
    mainLayout->addWidget(m_boardWidget, 1);     // 拉伸因子为1，占用剩余空间
    mainLayout->addLayout(buttonLayout, 0);      // 不拉伸

    // 设置窗口最小尺寸，确保棋盘有足够空间
    setMinimumSize(700, 800);

    // 样式设置
    setStyleSheet(R"(
        QDialog {
            background-color: #f5f5dc;
        }
        QLabel {
            color: #333333;
            padding: 5px;
        }
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 5px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
    )");
}
void ChessMainWindow::onGameStarted()
{
    updateStatus();

    // 启用/禁用按钮
    m_newGameBtn->setEnabled(false);
    m_exitBtn->setEnabled(true);

    QMessageBox::information(this, "游戏开始",
                             QString("游戏开始！您是%1方")
                                 .arg(m_humanColor == ChessPieceColor::Red ? "红" : "黑"));
}

void ChessMainWindow::onGameEnded(ChessPieceColor winner)
{
    QString message;
    if (winner == ChessPieceColor::Empty) {
        message = "游戏结束：平局！";
    } else if (winner == m_humanColor) {
        message = QString("恭喜您，%1方获胜！").arg(m_humanColor == ChessPieceColor::Red ? "红" : "黑");
    } else {
        message = QString("很遗憾，%1方获胜！").arg(m_humanColor == ChessPieceColor::Red ? "黑" : "红");
    }

    m_statusLabel->setText(message);

    // 启用新游戏按钮
    m_newGameBtn->setEnabled(true);

    QMessageBox::information(this, "游戏结束", message);
}

void ChessMainWindow::onNewGame()
{
    // 询问玩家选择哪一方
    QMessageBox::StandardButton reply = QMessageBox::question(this, "选择方",
                                                              "您想选择哪一方？\n\n红方（先手） - 点击 Yes\n黑方（后手） - 点击 No",
                                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (reply == QMessageBox::Cancel) {
        return;
    }

    m_humanColor = (reply == QMessageBox::Yes) ? ChessPieceColor::Red : ChessPieceColor::Black;

    // 重置状态
    m_statusLabel->setText("游戏初始化中...");

    // 初始化游戏
    m_boardWidget->initializeGame();

    updateStatus();
}

void ChessMainWindow::onExit()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认退出",
                                                              "确定要退出游戏吗？",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit returnToSelection();
        close();
    }
}

void ChessMainWindow::updateStatus()
{
    QString statusText;

    if (m_boardWidget) {
        ChessGameController* controller = m_boardWidget->getGameController();
        if (controller) {
            ChessPieceColor currentPlayer = controller->getCurrentPlayer();

            statusText = QString("当前玩家：%1 | 您是：%2方 | 状态：%3")
                             .arg(currentPlayer == ChessPieceColor::Red ? "红方" : "黑方")
                             .arg(m_humanColor == ChessPieceColor::Red ? "红" : "黑")
                             .arg(controller->isGameOver() ? "游戏结束" : "进行中");
        }
    }

    m_statusLabel->setText(statusText);
}

// 在关闭事件中发射信号
void ChessMainWindow::closeEvent(QCloseEvent* event) {
    emit closed();
    // QMainWindow::closeEvent(event);
}
