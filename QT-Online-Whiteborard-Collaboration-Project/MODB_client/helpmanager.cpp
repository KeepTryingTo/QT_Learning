#include "helpmanager.h"
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QTextEdit>
#include <QScrollArea>
#include <QDateTime>

#include "client.h"

// 版本信息常量
const QVersionNumber CURRENT_VERSION = QVersionNumber(1, 0, 0);
const QString BUILD_DATE = QString(__DATE__) + " " + QString(__TIME__);
const QString BUILD_PLATFORM =
#if defined(Q_OS_WIN)
    "Windows";
#elif defined(Q_OS_MAC)
    "macOS";
#elif defined(Q_OS_LINUX)
    "Linux";
#else
    "Unknown";
#endif

HelpManager::HelpManager(QObject *parent)
    : QObject(parent)
    , m_latestVersion("")
    , m_updateAvailable(false)
{
}

void HelpManager::showAboutDialog(QWidget *parent)
{
    AboutDialog dialog(parent);
    dialog.exec();
    emit aboutDialogShown();
}

void HelpManager::openHelpDocumentation()
{
    // 打开Qt官方文档
    QDesktopServices::openUrl(QUrl("https://doc.qt.io/qt-6/index.html"));
    emit helpDocumentationOpened();
}

QString HelpManager::getSoftwareInfo()
{
    return QString(
               "在线白板软件\n\n"
               "版本: 1.0.0\n"
               "构建时间: %1\n\n"
               "基于 Qt %2 开发\n\n"
               "功能特性:\n"
               "• 多人协作绘图\n"
               "• 实时同步功能\n"
               "• 多种绘图工具\n"
               "• 图层管理\n"
               "• 文件导入导出\n\n"
               "版权所有 © 2024 白板软件团队\n"
               "技术支持: support@whiteboard.com"
               ).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"))
        .arg(QT_VERSION_STR);
}

// AboutDialog 实现
HelpManager::AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("关于 - 在线白板软件");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(400, 500);

    setupUI();
}

void HelpManager::AboutDialog::setupUI()
{
    // 实例化一个垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 应用图标
    QLabel *iconLabel = new QLabel(this);
    QPixmap appIcon(":/../images/about.png"); // 使用资源文件中的图标
    if (appIcon.isNull()) {
        // 如果没有图标资源，使用默认图标
        appIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(64, 64);
    }
    iconLabel->setPixmap(appIcon);
    iconLabel->setAlignment(Qt::AlignCenter);

    // 软件信息
    QLabel *infoLabel = new QLabel(HelpManager::getSoftwareInfo(), this);
    infoLabel->setWordWrap(true);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("QLabel { background-color: white; padding: 10px; border-radius: 5px; }");

    // 版权信息
    QLabel *copyrightLabel = new QLabel(
        "本软件基于GPL v3开源协议发布\n"
        "更多信息请访问: https://github.com/KeepTryingTo/QT_Online-Whiteborard-Collaboration-Project", this);
    copyrightLabel->setWordWrap(true);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; }");

    // 确定按钮
    QPushButton *okButton = new QPushButton("确定", this);
    okButton->setFixedWidth(100);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    // 添加到布局
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(copyrightLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
}


// 版本信息相关方法
QString HelpManager::getVersionString()
{
    return CURRENT_VERSION.toString();
}

QVersionNumber HelpManager::getVersionNumber()
{
    return CURRENT_VERSION;
}

QString HelpManager::getBuildDate()
{
    return BUILD_DATE;
}

QString HelpManager::getBuildInfo()
{
    return QString("Built on %1 for %2 with Qt %3")
    .arg(BUILD_DATE)
        .arg(BUILD_PLATFORM)
        .arg(QT_VERSION_STR);
}

void HelpManager::showVersionDialog(QWidget *parent)
{
    VersionDialog dialog(parent);
    dialog.exec();
    emit versionDialogShown();
}

// 检查更新功能
void HelpManager::checkForUpdates(bool silent)
{
    emit updateCheckStarted();

    // 这里可以实现实际的更新检查逻辑

    // 模拟网络请求（实际项目中应该使用真实的API）
    QTimer::singleShot(2000, this, [this, silent]() {
        // 模拟获取到新版本
        m_latestVersion = "1.1.0";
        m_updateAvailable = (QVersionNumber::fromString(m_latestVersion) > CURRENT_VERSION);

        emit updateCheckFinished(m_updateAvailable, m_latestVersion);

        if (m_updateAvailable && !silent) {
            emit updateAvailable(getVersionString(), m_latestVersion);

            // 显示更新提示
            QMessageBox::information(nullptr, "发现新版本",
                                     QString("发现新版本 %1\n当前版本: %2\n\n请访问官网下载最新版本。")
                                         .arg(m_latestVersion)
                                         .arg(getVersionString()));
        } else if (!silent) {
            QMessageBox::information(nullptr, "版本检查", "当前已是最新版本。");
        }
    });
}

bool HelpManager::isUpdateAvailable() const
{
    return m_updateAvailable;
}

QString HelpManager::getLatestVersion() const
{
    return m_latestVersion;
}

// VersionDialog 实现
HelpManager::VersionDialog::VersionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("版本信息 - 在线白板软件");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(500, 400);

    setupUI();
}

void HelpManager::VersionDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 应用信息组
    QGroupBox *appGroup = new QGroupBox("应用程序信息", this);
    QGridLayout *appLayout = new QGridLayout(appGroup);

    appLayout->addWidget(new QLabel("软件名称:"), 0, 0);
    appLayout->addWidget(new QLabel("在线白板软件"), 0, 1);

    appLayout->addWidget(new QLabel("当前版本:"), 1, 0);
    appLayout->addWidget(new QLabel(HelpManager::getVersionString()), 1, 1);

    appLayout->addWidget(new QLabel("构建时间:"), 2, 0);
    appLayout->addWidget(new QLabel(HelpManager::getBuildDate()), 2, 1);

    appLayout->addWidget(new QLabel("构建平台:"), 3, 0);
    appLayout->addWidget(new QLabel(BUILD_PLATFORM), 3, 1);

    appLayout->addWidget(new QLabel("Qt 版本:"), 4, 0);
    appLayout->addWidget(new QLabel(QT_VERSION_STR), 4, 1);

    appGroup->setLayout(appLayout);

    // 系统信息组
    QGroupBox *sysGroup = new QGroupBox("系统信息", this);
    QGridLayout *sysLayout = new QGridLayout(sysGroup);

    sysLayout->addWidget(new QLabel("操作系统:"), 0, 0);
    sysLayout->addWidget(new QLabel(QSysInfo::prettyProductName()), 0, 1);

    sysLayout->addWidget(new QLabel("系统架构:"), 1, 0);
    sysLayout->addWidget(new QLabel(QSysInfo::currentCpuArchitecture()), 1, 1);

    sysLayout->addWidget(new QLabel("内核版本:"), 2, 0);
    sysLayout->addWidget(new QLabel(QSysInfo::kernelVersion()), 2, 1);

    sysGroup->setLayout(sysLayout);

    // 更新检查按钮
    QPushButton *checkUpdateButton = new QPushButton("检查更新", this);
    checkUpdateButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));

    // 确定按钮
    QPushButton *okButton = new QPushButton("确定", this);
    okButton->setFixedWidth(100);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(checkUpdateButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);

    // 连接到父窗口的HelpManager实例
    // 通过parent()获取Client实例，然后获取HelpManager
    QObject *parentObj = parent();
    while (parentObj && !qobject_cast<Client*>(parentObj)) {
        parentObj = parentObj->parent();
    }

    if (Client *client = qobject_cast<Client*>(parentObj)) {
        connect(checkUpdateButton, &QPushButton::clicked, this, [client]() {
            client->getHelpManager()->checkForUpdates(false);
        });
    } else {
        // 如果找不到Client实例，禁用按钮
        checkUpdateButton->setEnabled(false);
        checkUpdateButton->setToolTip("无法连接到主应用程序");
    }

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    // 添加到主布局
    mainLayout->addWidget(appGroup);
    mainLayout->addWidget(sysGroup);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

