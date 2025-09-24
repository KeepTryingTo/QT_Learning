#include "filemanager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QFileInfo>

FileManager::FileManager(QObject *parent)
    : QObject(parent), m_currentFilePath(""), m_isModified(false), m_isLoading(false)
{
}

bool FileManager::newFile(QGraphicsScene *scene)
{
    if (maybeSave(scene)) {
        // 清除原来的内容，然后发送一个信号
        scene->clear();
        m_currentFilePath = "";
        m_isModified = false;
        emit fileModified(false);
        return true;
    }
    return false;
}

bool FileManager::openFile(QGraphicsScene *scene, const QString &fileName)
{
    QString fileToOpen = fileName;
    // 如果提供了文件名，直接使用；否则弹出对话框
    if (fileToOpen.isEmpty()) {
        fileToOpen = QFileDialog::getOpenFileName(nullptr,
                                                  "打开白板文件", "", "白板文件 (*.wb);;所有文件 (*)");

        // 如果用户取消了选择，直接返回
        if (fileToOpen.isEmpty()) {
            return false;
        }
    }
    // 加载打开的白板文件
    if (!fileToOpen.isEmpty()) {
        if (loadFromFile(scene, fileToOpen)) {
            m_currentFilePath = fileToOpen;
            m_isModified = false;
            // 打开成功之后也发送一个信号给client
            emit fileOpened(fileToOpen);
            emit fileModified(false);
            return true;
        }
    }
    return false;
}

bool FileManager::saveFile(QGraphicsScene *scene, const QString &fileName)
{
    if (fileName.isEmpty() && m_currentFilePath.isEmpty()) {
        return saveAsFile(scene);
    }

    QString fileToSave = fileName.isEmpty() ? m_currentFilePath : fileName;
    if (saveToFile(scene, fileToSave)) {
        m_currentFilePath = fileToSave;
        m_isModified = false;
        emit fileSaved(fileToSave);
        emit fileModified(false);
        return true;
    }
    return false;
}

bool FileManager::saveAsFile(QGraphicsScene *scene, const QString &fileName)
{
    QString fileToSave = fileName;
    if (fileToSave.isEmpty()) {
        fileToSave = QFileDialog::getSaveFileName(nullptr,
                                                  "保存白板文件", "", "白板文件 (*.wb);;所有文件 (*)");
    }

    if (!fileToSave.isEmpty()) {
        if (!fileToSave.endsWith(".wb", Qt::CaseInsensitive)) {
            fileToSave += ".wb";
        }
        return saveFile(scene, fileToSave);
    }
    return false;
}

bool FileManager::exportToImage(QGraphicsScene *scene, const QString &fileName)
{
    QString fileToExport = fileName;
    if (fileToExport.isEmpty()) {
        fileToExport = QFileDialog::getSaveFileName(nullptr,
                                                    "导出为图片", "", "PNG图像 (*.png);;JPEG图像 (*.jpg);;所有文件 (*)");
    }

    if (!fileToExport.isEmpty()) {
        exportSceneToImage(scene, fileToExport);
        return true;
    }
    return false;
}

bool FileManager::maybeSave(QGraphicsScene *scene)
{
    // 首先判断是否进行了修改，如果进行了修改就保存
    if (m_isModified) {
        QMessageBox::StandardButton ret = QMessageBox::warning(nullptr, "白板",
                                                               "文档已被修改。\n是否保存更改？",
                                                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        // 保存或者不保存修改内容
        if (ret == QMessageBox::Save) {
            return saveFile(scene);
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

// 私有实现方法
bool FileManager::saveToFile(QGraphicsScene *scene, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("无法保存文件");
        return false;
    }

    // 对白板内容进行序列化
    QJsonArray itemsArray = serializeScene(scene);
    QJsonDocument doc(itemsArray);

    // 将序列化的内容保存到文件中
    if (file.write(doc.toJson()) == -1) {
        emit errorOccurred("写入文件失败");
        return false;
    }

    file.close();
    return true;
}

bool FileManager::loadFromFile(QGraphicsScene *scene, const QString &fileName)
{
     m_isLoading = true;  // 设置加载状态

    // 以只读的方式打开文件
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法打开文件");
        return false;
    }

    // 读取所有的内容并JSON化
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isArray()) {
        emit errorOccurred("文件格式错误");
        return false;
    }
    // 清除原来的内容之后使用打开的文件内容代替
    scene->clear();
    // 对序列化的内容进行反序列化，得到原始的字符
    deserializeScene(scene, doc.array());

    m_isLoading = false;
    return true;
}

void FileManager::exportSceneToImage(QGraphicsScene *scene, const QString &fileName)
{
    // 获取QGraphicsScene（场景）的矩形边界
    QRectF rect = scene->sceneRect();
    QPixmap pixmap(rect.width(), rect.height());
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    // 为QPainter设置渲染提示（Render Hint），这里启用了抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);
    /*
        它指挥QGraphicsScene将其整个内容（包括所有的图形项，如线条、形状、文本、图片等）
        渲染到我们之前创建的QPainter中。而由于QPainter是与QPixmap关联的，
        所以场景的内容就被“画”到了pixmap这个内存中的图像上
    */
    scene->render(&painter);
    // 按照图像的方式导出来
    if (!pixmap.save(fileName)) {
        emit errorOccurred("无法导出图片");
    }
}

QJsonArray FileManager::serializeScene(QGraphicsScene *scene)
{
    QJsonArray itemsArray;

    // 添加网格状态信息
    QJsonObject gridInfo;
    gridInfo["type"] = "gridInfo";
    gridInfo["showGrid"] = m_showGrid;
    itemsArray.append(gridInfo);

    foreach (QGraphicsItem *item, scene->items()) {
        QJsonObject itemObject;

        // 跳过网格项（不保存网格线）
        if (item->data(Qt::UserRole).toString() == "grid") {
            continue;
        }

        if (QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem*>(item)) {
            itemObject["type"] = "line";
            itemObject["x1"] = line->line().x1();
            itemObject["y1"] = line->line().y1();
            itemObject["x2"] = line->line().x2();
            itemObject["y2"] = line->line().y2();
            itemObject["color"] = line->pen().color().name();
            itemObject["width"] = line->pen().width();
        }
        else if (QGraphicsRectItem *rect = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
            itemObject["type"] = "rect";
            itemObject["x"] = rect->rect().x();
            itemObject["y"] = rect->rect().y();
            itemObject["width"] = rect->rect().width();
            itemObject["height"] = rect->rect().height();
            itemObject["color"] = rect->pen().color().name();

            // 保存填充信息和填充状态
            if (rect->brush().style() != Qt::NoBrush) {
                itemObject["fillColor"] = rect->brush().color().name();
                itemObject["hasFill"] = true;
            } else {
                itemObject["hasFill"] = false;
            }
        }
        else if (QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
            itemObject["type"] = "ellipse";
            itemObject["x"] = ellipse->rect().x();
            itemObject["y"] = ellipse->rect().y();
            itemObject["width"] = ellipse->rect().width();
            itemObject["height"] = ellipse->rect().height();
            itemObject["color"] = ellipse->pen().color().name();

            // 保存填充信息和填充状态
            if (ellipse->brush().style() != Qt::NoBrush) {
                itemObject["fillColor"] = ellipse->brush().color().name();
                itemObject["hasFill"] = true;
            } else {
                itemObject["hasFill"] = false;
            }
        }
        else if (QGraphicsTextItem *text = qgraphicsitem_cast<QGraphicsTextItem*>(item)) {
            itemObject["type"] = "text";
            itemObject["x"] = text->pos().x();
            itemObject["y"] = text->pos().y();
            itemObject["content"] = text->toPlainText();
            itemObject["color"] = text->defaultTextColor().name();
        }

        itemsArray.append(itemObject);
    }

    return itemsArray;
}

void FileManager::deserializeScene(QGraphicsScene *scene, const QJsonArray &itemsArray)
{
    bool showGrid = true;  // 默认显示网格

    // 先清除非网格内容
    QList<QGraphicsItem*> items = scene->items();
    foreach (QGraphicsItem *item, items) {
        if (item->data(Qt::UserRole).toString() != "grid") {
            scene->removeItem(item);
        }
    }

    foreach (const QJsonValue &value, itemsArray) {
        QJsonObject itemObject = value.toObject();
        QString type = itemObject["type"].toString();

        if (type == "gridInfo") {
            // 读取网格状态
            showGrid = itemObject["showGrid"].toBool();
            m_showGrid = showGrid;
            continue;
        }

        if (type == "line") {
            QGraphicsLineItem *line = new QGraphicsLineItem(
                itemObject["x1"].toDouble(),
                itemObject["y1"].toDouble(),
                itemObject["x2"].toDouble(),
                itemObject["y2"].toDouble()
                );
            QPen pen(QColor(itemObject["color"].toString()));
            pen.setWidth(itemObject["width"].toInt());
            line->setPen(pen);
            scene->addItem(line);
        }
        else if (type == "rect") {
            QGraphicsRectItem *rect = new QGraphicsRectItem(
                itemObject["x"].toDouble(),
                itemObject["y"].toDouble(),
                itemObject["width"].toDouble(),
                itemObject["height"].toDouble()
                );
            QPen pen(QColor(itemObject["color"].toString()));
            rect->setPen(pen);

            // 根据hasFill字段决定是否设置填充
            if (itemObject.contains("hasFill") && itemObject["hasFill"].toBool()) {
                rect->setBrush(QColor(itemObject["fillColor"].toString()));
            } else {
                rect->setBrush(Qt::NoBrush); // 明确设置为无填充
            }

            scene->addItem(rect);
        }
        else if (type == "ellipse") {
            QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(
                itemObject["x"].toDouble(),
                itemObject["y"].toDouble(),
                itemObject["width"].toDouble(),
                itemObject["height"].toDouble()
                );
            QPen pen(QColor(itemObject["color"].toString()));
            ellipse->setPen(pen);

            // 根据hasFill字段决定是否设置填充
            if (itemObject.contains("hasFill") && itemObject["hasFill"].toBool()) {
                ellipse->setBrush(QColor(itemObject["fillColor"].toString()));
            } else {
                ellipse->setBrush(Qt::NoBrush); // 明确设置为无填充
            }

            scene->addItem(ellipse);
        }
        else if (type == "text") {
            QGraphicsTextItem *text = new QGraphicsTextItem(
                itemObject["content"].toString()
                );
            text->setPos(
                itemObject["x"].toDouble(),
                itemObject["y"].toDouble()
                );
            text->setDefaultTextColor(QColor(itemObject["color"].toString()));
            scene->addItem(text);
        }
    }
}

// Getter 方法
QString FileManager::currentFilePath() const { return m_currentFilePath; }
bool FileManager::isModified() const { return m_isModified; }
void FileManager::setModified(bool modified) {
    m_isModified = modified;
    if (!m_isLoading) {  // 只有在非加载状态下才设置修改状态
        m_isModified = modified;
    }
}

void FileManager::setSceneCleared()
{
    m_isModified = true;  // 清除操作也视为修改
    emit fileModified(true);
}
