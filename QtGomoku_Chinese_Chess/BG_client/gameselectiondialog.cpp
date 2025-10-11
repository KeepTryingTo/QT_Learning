// GameSelectionDialog.cpp
#include "GameSelectionDialog.h"
#include <QButtonGroup>
#include <QMessageBox>

GameSelectionDialog::GameSelectionDialog(const QString &username, QWidget *parent)
    : QDialog(parent), m_username(username), m_selectedGame(""), m_selectedMode("") {

    setWindowTitle("游戏选择");
    setFixedSize(500, 500);
    QIcon select(":/../images/login.png");
    setWindowIcon(select);

    // 移除默认窗口标志，允许拖动
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    setupUI();
    applyStyle();
}

void GameSelectionDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(20);

    // 欢迎标题
    m_labelWelcome = new QLabel(QString("欢迎，%1！请选择游戏").arg(m_username));
    m_labelWelcome->setAlignment(Qt::AlignCenter);

    // 游戏选择区域
    QWidget *gameSection = new QWidget;
    QVBoxLayout *gameLayout = new QVBoxLayout(gameSection);

    m_labelGame = new QLabel("选择游戏类型:");
    m_labelGame->setAlignment(Qt::AlignLeft);

    QHBoxLayout *gameButtonsLayout = new QHBoxLayout;
    m_btnGomoku = new QPushButton("五子棋");
    m_btnChess = new QPushButton("中国象棋");

    m_btnGomoku->setCheckable(true);
    m_btnChess->setCheckable(true);

    gameButtonsLayout->addWidget(m_btnGomoku);
    gameButtonsLayout->addWidget(m_btnChess);

    m_gameGroup = new QButtonGroup(this);
    m_gameGroup->addButton(m_btnGomoku, 0);
    m_gameGroup->addButton(m_btnChess, 1);

    gameLayout->addWidget(m_labelGame);
    gameLayout->addLayout(gameButtonsLayout);

    // 对战模式选择区域
    QWidget *modeSection = new QWidget;
    QVBoxLayout *modeLayout = new QVBoxLayout(modeSection);

    m_labelMode = new QLabel("选择对战模式:");
    m_labelMode->setAlignment(Qt::AlignLeft);

    QHBoxLayout *modeButtonsLayout = new QHBoxLayout;
    m_btnPvP = new QPushButton("人人对战");
    m_btnPvE = new QPushButton("人机对战");


    m_btnPvP->setCheckable(true);
    m_btnPvE->setCheckable(true);


    modeButtonsLayout->addWidget(m_btnPvP);
              // 弹性空间
    modeButtonsLayout->addWidget(m_btnPvE);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(m_btnPvP, 0);
    m_modeGroup->addButton(m_btnPvE, 1);

    modeLayout->addWidget(m_labelMode);
    modeLayout->addLayout(modeButtonsLayout);

    // 操作按钮
    QHBoxLayout *startLayout = new QHBoxLayout;
    m_btnStart = new QPushButton("开始游戏");
    m_btnStart->setEnabled(false);

    startLayout->addStretch();
    startLayout->addWidget(m_btnStart);
    startLayout->addStretch();

    // 第二行：返回登录 + 退出程序按钮
    QHBoxLayout *actionLayout = new QHBoxLayout;
    m_btnBack = new QPushButton("返回登录");
    m_btnExit = new QPushButton("退出程序");

    actionLayout->addWidget(m_btnBack);
    actionLayout->addStretch();  // 弹性空间
    actionLayout->addWidget(m_btnExit);

    // 组装主界面
    mainLayout->addWidget(m_labelWelcome);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(gameSection);
    mainLayout->addWidget(modeSection);
    mainLayout->addStretch();    // 弹性空间
    mainLayout->addLayout(startLayout);   // 开始游戏按钮
    mainLayout->addLayout(actionLayout);  // 返回登录 + 退出程序按钮

    // 连接信号（保持不变）
    connect(m_gameGroup, &QButtonGroup::buttonClicked, this, &GameSelectionDialog::onGameSelected);
    connect(m_modeGroup, &QButtonGroup::buttonClicked, this, &GameSelectionDialog::onModeSelected);
    connect(m_btnStart, &QPushButton::clicked, this, &GameSelectionDialog::onStartClicked);
    connect(m_btnBack, &QPushButton::clicked, this, &GameSelectionDialog::onBackClicked);
    connect(m_btnExit, &QPushButton::clicked, this, &GameSelectionDialog::onExitClicked);
}

void GameSelectionDialog::applyStyle() {
    QFile file(":/resources/select.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }
    // 设置欢迎标签的特殊样式
    m_labelWelcome->setObjectName("welcomeLabel");
}

void GameSelectionDialog::onGameSelected(QAbstractButton *button) {
    if (button == m_btnGomoku) {
        m_selectedGame = "gomoku";
    } else if (button == m_btnChess) {
        m_selectedGame = "chess";
    }
    updateStartButton();
}

void GameSelectionDialog::onModeSelected(QAbstractButton *button) {
    if (button == m_btnPvP) {
        m_selectedMode = "pvp";
    } else if (button == m_btnPvE) {
        m_selectedMode = "pve";
    }
    updateStartButton();
}

void GameSelectionDialog::updateStartButton() {
    // 只有当选择了游戏类型和对战模式后才启用开始按钮
    m_btnStart->setEnabled(!m_selectedGame.isEmpty() && !m_selectedMode.isEmpty());
}

void GameSelectionDialog::onStartClicked() {
    QString message = QString("即将开始游戏：%1 - %2模式")
                          .arg(m_selectedGame == "gomoku" ? "五子棋" : "中国象棋")
                          .arg(m_selectedMode == "pvp" ? "人人对战" : "人机对战");

    QMessageBox::information(this, "游戏准备", message);

    // 这里可以跳转到对应的游戏界面
    accept(); // 关闭选择对话框，进入游戏
}

void GameSelectionDialog::onBackClicked() {
    reject(); // 返回登录界面
}

void GameSelectionDialog::onExitClicked() {
    // 退出程序确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认退出",
                                  "确定要退出游戏吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // QCoreApplication::quit();  // 退出整个应用程序
        // reject();
        done(2); // 使用自定义返回码
    }
}
