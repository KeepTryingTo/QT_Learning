#ifndef HELPMANAGER_H
#define HELPMANAGER_H

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QGroupBox>
#include <QMessageBox>
#include <QTimer>
#include <QVersionNumber>

class HelpManager : public QObject
{
    Q_OBJECT

public:
    explicit HelpManager(QObject *parent = nullptr);



    // 显示关于对话框
    void showAboutDialog(QWidget *parent = nullptr);

    // 显示版本信息对话框
    void showVersionDialog(QWidget *parent = nullptr);

    // 打开帮助文档
    void openHelpDocumentation();

    // 获取软件信息
    static QString getSoftwareInfo();


    // 版本管理相关方法
    static QString getVersionString();
    static QVersionNumber getVersionNumber();
    static QString getBuildDate();
    static QString getBuildInfo();

    // 检查更新相关方法
    void checkForUpdates(bool silent = false);
    bool isUpdateAvailable() const;
    QString getLatestVersion() const;

signals:
    void aboutDialogShown();
    void versionDialogShown();
    void helpDocumentationOpened();
    void updateCheckStarted();
    void updateCheckFinished(bool updateAvailable, const QString &latestVersion);
    void updateAvailable(const QString &currentVersion, const QString &latestVersion);

private:
    // 关于对话框类
    class AboutDialog : public QDialog
    {
    public:
        AboutDialog(QWidget *parent = nullptr);

    private:
        void setupUI();
    };

    // 版本信息对话框类
    class VersionDialog : public QDialog
    {
    public:
        VersionDialog(QWidget *parent = nullptr);

    private:
        void setupUI();
    };

    QString m_latestVersion;
    bool m_updateAvailable;
};

#endif // HELPMANAGER_H
