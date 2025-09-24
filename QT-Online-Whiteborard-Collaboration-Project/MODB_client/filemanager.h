#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QJsonArray>

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);

    // 文件操作
    bool newFile(QGraphicsScene *scene);
    bool openFile(QGraphicsScene *scene, const QString &fileName = "");
    bool saveFile(QGraphicsScene *scene, const QString &fileName = "");
    bool saveAsFile(QGraphicsScene *scene, const QString &fileName = "");
    bool exportToImage(QGraphicsScene *scene, const QString &fileName = "");

    // 状态获取
    QString currentFilePath() const;
    bool isModified() const;
    void setModified(bool modified);
    bool getModified(){
        return this -> m_isModified;
    }

    // 检查是否需要保存
    bool maybeSave(QGraphicsScene *scene);
    // 白板内容被清除时调用
    void setSceneCleared();

signals:
    void fileOpened(const QString &fileName);
    void fileSaved(const QString &fileName);
    void fileModified(bool modified);
    void errorOccurred(const QString &message);
    void gridStateChanged(bool showGrid);

private:
    // 内部实现方法
    bool saveToFile(QGraphicsScene *scene, const QString &fileName);
    bool loadFromFile(QGraphicsScene *scene, const QString &fileName);
    void exportSceneToImage(QGraphicsScene *scene, const QString &fileName);

    // 序列化和反序列化方法,主要用于保存文件以及打开文件
    QJsonArray serializeScene(QGraphicsScene *scene);
    void deserializeScene(QGraphicsScene *scene, const QJsonArray &itemsArray);

    QString m_currentFilePath;
    bool m_isModified;
    bool m_isLoading;  // 添加加载状态标志

    bool m_showGrid;  // 网格显示状态
};

#endif // FILEMANAGER_H
