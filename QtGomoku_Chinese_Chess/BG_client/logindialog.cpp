// LoginDialog.cpp
#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpacerItem>
#include <QGraphicsDropShadowEffect>
#include <QCryptographicHash>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("五子棋 - 登录");
    // setFixedSize(400, 500);
    // 加载CSS样式
    QFile file(":/resources/login.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }
    setWindowFlags(Qt::FramelessWindowHint); // 无边框
    // 移除默认窗口标志，允许拖动
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QIcon select(":/../images/select.png");
    setWindowIcon(select);

    applyModernStyle();
    initDatabase();
}

void LoginDialog::applyModernStyle(){
    setWindowTitle("登录");
    setFixedSize(300, 300);  // 稍微增加高度以容纳新按钮

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 15, 20, 15);
    layout->setSpacing(10);

    // 用户名输入
    QLabel *labelUser = new QLabel("用户名:");
    m_editUsername = new QLineEdit;
    m_editUsername->setPlaceholderText("请输入用户名");
    m_editUsername->setMinimumHeight(30);

    // 密码输入
    QLabel *labelPwd = new QLabel("密码:");
    m_editPassword = new QLineEdit;
    m_editPassword->setPlaceholderText("请输入密码");
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setMinimumHeight(30);

    // 按钮布局 - 改为两行布局
    QHBoxLayout *firstBtnLayout = new QHBoxLayout;
    QHBoxLayout *secondBtnLayout = new QHBoxLayout;

    QPushButton *btnLogin = new QPushButton("登录");
    QPushButton *btnRegister = new QPushButton("注册");
    QPushButton *btnExit = new QPushButton("退出");

    // 设置按钮大小
    btnLogin->setMinimumHeight(35);
    btnRegister->setMinimumHeight(35);
    btnExit->setMinimumHeight(35);

    // 第一行按钮：登录 + 注册
    firstBtnLayout->addWidget(btnLogin);
    firstBtnLayout->addWidget(btnRegister);

    // 第二行按钮：退出（居中显示）
    secondBtnLayout->addStretch();  // 左侧弹性空间
    secondBtnLayout->addWidget(btnExit);
    secondBtnLayout->addStretch();  // 右侧弹性空间

    // 添加到主布局
    // layout->addWidget(new QLabel("登录"));  // 添加登录标题
    layout->addSpacing(10);  // 添加间距
    layout->addWidget(labelUser);
    layout->addWidget(m_editUsername);
    layout->addWidget(labelPwd);
    layout->addWidget(m_editPassword);
    layout->addSpacing(10);  // 添加间距
    layout->addLayout(firstBtnLayout);
    layout->addLayout(secondBtnLayout);

    // 连接信号
    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(btnRegister, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(btnExit, &QPushButton::clicked, this, &LoginDialog::onExitClicked);

    // 设置Tab键顺序
    setTabOrder(m_editUsername, m_editPassword);
    setTabOrder(m_editPassword, btnLogin);
    setTabOrder(btnLogin, btnRegister);
    setTabOrder(btnRegister, btnExit);
}

void LoginDialog::onExitClicked() {
    // 发送退出信号，让整个应用程序退出
    QCoreApplication::quit();
    reject();
}

void LoginDialog::onTogglePassword() {
    static bool passwordVisible = false;
    passwordVisible = !passwordVisible;

    m_editPassword->setEchoMode(passwordVisible ? QLineEdit::Normal : QLineEdit::Password);

    // 更新按钮图标
    QString style = this->styleSheet();
    if (passwordVisible) {
        style.replace("togglePasswordBtn { background-image: url('data:image/svg+xml,<svg xmlns",
                      "togglePasswordBtn { background-image: url('data:image/svg+xml,<svg xmlns");
        // 这里可以切换为隐藏密码的图标
    }
    setStyleSheet(style);
}

void LoginDialog::onForgotPassword() {
    QMessageBox::information(this, "忘记密码", "请联系系统管理员重置密码");
}

void LoginDialog::loadUserData() {
    user_info.clear();  // 清空现有数据

    QSqlQuery query("SELECT username, password FROM user_info");
    if (!query.exec()) {
        QMessageBox::warning(this, "警告", "加载用户数据失败: " + query.lastError().text());
        return;
    }

    int count = 0;
    while (query.next()) {
        QString username = query.value(0).toString();
        QString password = query.value(1).toString();
        user_info[username] = password;  // 存储到map中
        count++;
    }

    qDebug() << "成功加载" << count << "个用户数据到内存";
}


void LoginDialog::initDatabase() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("D:\\SoftwareFamily\\SQLite\\projects\\boardGame.db");

    if (!m_db.open()) {
        QMessageBox::critical(this, "错误", "数据库连接失败: " + m_db.lastError().text());
    } else {
        // 数据库连接成功后立即加载用户数据到内存
        loadUserData();
        QMessageBox::information(this, "提示", "数据库连接成功，已加载用户数据", QMessageBox::Ok);
    }
}

bool LoginDialog::userExists(const QString &username) {
    // 直接从内存map中检查
    return user_info.find(username) != user_info.end();
}

QString LoginDialog::hashPassword(const QString &password) {
    // 如果您需要密码加密，可以在这里实现
    // 目前直接返回明文，因为您的数据库存储的是明文
    return password;

    // 如果需要加密，取消下面的注释：
    // return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
}

void LoginDialog::onTextChanged() {
    // 只有用户名和密码都不为空时才启用登录按钮
    bool enable = !m_editUsername->text().trimmed().isEmpty() &&
                  !m_editPassword->text().isEmpty();
}

void LoginDialog::onLoginClicked() {
    QString username = m_editUsername->text().trimmed();
    QString password = m_editPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入完整的登录信息");
        return;
    }

    // 直接从内存map中查询，不访问数据库
    auto it = user_info.find(username);
    if (it != user_info.end()) {
        QString storedPassword = it->second;
        if (storedPassword == password) {  // 注意：这里直接比较明文密码
            accept();
        } else {
            QMessageBox::warning(this, "登录失败", "密码错误，请重新输入");
            m_editPassword->clear();
            m_editPassword->setFocus();
        }
    } else {
        QMessageBox::warning(this, "登录失败", "用户名不存在");
    }
}

void LoginDialog::onRegisterClicked() {
    QString username = m_editUsername->text().trimmed();
    QString password = m_editPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入完整的注册信息");
        return;
    }

    // 从内存map中检查用户名是否存在
    if (user_info.find(username) != user_info.end()) {
        QMessageBox::warning(this, "注册失败", "用户名已存在，请选择其他用户名");
        return;
    }

    // 插入到数据库
    QSqlQuery query;
    query.prepare("INSERT INTO user_info (username, password) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(password);  // 注意：这里存储的是明文密码

    if (query.exec()) {
        // 同时更新内存map
        user_info[username] = password;
        QMessageBox::information(this, "注册成功", "账号注册成功，请登录");
        m_editPassword->clear();
    } else {
        QMessageBox::critical(this, "注册失败", "注册失败，请重试: " + query.lastError().text());
    }
}
