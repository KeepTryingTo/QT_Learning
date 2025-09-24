// drawingtool.cpp
#include "drawingtool.h"
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPen>

DrawingTool::DrawingTool(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene), m_currentTool(Pencil), m_tempItem(nullptr),
    m_isOnlineMode(false) // 默认离线模式
    , m_currentNetworkPath(nullptr)
{
    // 初始化画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(2);
    m_pen.setStyle(Qt::SolidLine);
    m_pen.setCapStyle(Qt::RoundCap);
    m_pen.setJoinStyle(Qt::RoundJoin);

    m_brush.setColor(Qt::transparent);
    m_brush.setStyle(Qt::NoBrush);

    is_full = false;
    pen_width = 1;

    m_isErasing = false;
    m_erasedItems.clear();

    m_currentPath = nullptr;
    font_weight = 12;
}

void DrawingTool::setCurrentTool(ToolType tool)
{
    // 清理之前的临时项
    if (m_tempItem) {
        m_scene->removeItem(m_tempItem);
        delete m_tempItem;
        m_tempItem = nullptr;
    }

    // 重置橡皮擦状态
    if (m_currentTool == Eraser) {
        m_isErasing = false;
    }
    // 设置当前操作工具，比如文本，绘制矩形，椭圆等类型
    m_currentTool = tool;
    emit toolChanged(tool);
}

void DrawingTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 绘制图形首先按下的是左键
    if (event->button() != Qt::LeftButton) return;

    // 获得当前的起始坐标
    m_startPoint = event->scenePos();
    m_isDrawing = true;

    switch (m_currentTool) {
    case Line:
        drawLine(event);
        break;
    case Rectangle:
        drawRectangle(event);
        break;
    case Ellipse:
        drawEllipse(event);
        break;
    case Text:
        drawText(event);
        break;
    case Eraser:
        m_isErasing = true;
        // 保存橡皮擦操作前的状态
        saveState();
        // 显示橡皮擦预览
        drawEraser(event);
        // 执行擦除操作
        eraseAtPosition(event->scenePos());
        break;
    default:
        break;
    }
}

void DrawingTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // 鼠标移动的终点坐标位置
    m_endPoint = event->scenePos();

    switch (m_currentTool) {
    case Pencil:
        drawPencil(event);
        break;
    case Line:
        if (m_tempItem) {
            QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem*>(m_tempItem);
            line->setLine(QLineF(m_startPoint, m_endPoint));
            line->setPen(m_pen);
        }
        break;
    case Rectangle:
        if (m_tempItem) {
            QGraphicsRectItem *rect = qgraphicsitem_cast<QGraphicsRectItem*>(m_tempItem);
            QRectF newRect(m_startPoint, m_endPoint);
            rect->setRect(newRect.normalized());
            rect->setPen(m_pen);
        }
        break;
    case Ellipse:
        if (m_tempItem) {
            QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(m_tempItem);
            QRectF newRect(m_startPoint, m_endPoint);
            ellipse->setRect(newRect.normalized());
            ellipse->setPen(m_pen);
        }
        break;
    case Eraser:
        // 显示橡皮擦预览
        drawEraser(event);
        // 如果正在擦除，执行擦除操作
        if (m_isErasing && (event->buttons() & Qt::LeftButton)) {
            eraseAtPosition(event->scenePos());
        }
        break;
    default:
        break;
    }
}

void DrawingTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_endPoint = event->scenePos();
    m_isDrawing = false;

    if (m_currentTool == Eraser) {
        m_isErasing = false;
        if (m_tempItem) {
            m_scene->removeItem(m_tempItem);
            delete m_tempItem;
            m_tempItem = nullptr;
        }

        // 橡皮擦操作完成，保存状态
        saveState();

        // 如果是网络模式，发送橡皮擦操作
        if (m_isOnlineMode) {
            DrawingOperation operation;
            operation.opType = DOT_Erase;
            operation.data = QVariantMap{
                {"positionX", event->scenePos().x()},
                {"positionY", event->scenePos().y()},
                {"eraserSize", m_pen.width() + 10}
            };
            // 只要绘制之后，鼠标释放之后就会通过消息发送给客户端，然后将消息自动发送给服务端，并广播其他客户端
            emit drawingOperationCreated(operation);
        }
        // 通知内容已修改
        emit contentModified();
    }
    else if (m_tempItem && m_currentTool != Pencil && m_currentTool != Eraser) {
        // 对于形状工具，释放鼠标时完成绘制
        QGraphicsItem* finishedItem = m_tempItem;
        m_tempItem = nullptr;

        // 确保当前绘制的结果被正确添加到场景
        if (!m_scene->items().contains(finishedItem)) {
            m_scene->addItem(finishedItem);
        }

        // 如果是网络模式，发送绘图操作
        if (m_isOnlineMode) {
            DrawingOperation operation;
            std::cout<<"m_currentTool = "<<m_currentTool<<std::endl;
            // 获得当前的状态，然后执行相应的操作
            operation.opType = getCurrentOperationType();
            operation.data = getCurrentOperationData(finishedItem);
            emit drawingOperationCreated(operation);
        }

        // 保存当前状态（用于撤销）
        saveState();

        // 通知内容已修改
        emit contentModified();
    }
    else if (m_currentTool == Pencil && m_currentPath) {
        // 铅笔绘图完成
        if (m_isOnlineMode) {
            DrawingOperation operation;
            operation.opType = DOT_EndStroke;// 结束笔画

            operation.data = QVariantMap();
            operation.data.insert("path", QVariant::fromValue(m_currentPath->path()));
            operation.data.insert("penColor", m_pen.color());
            operation.data.insert("penWidth", m_pen.width());

            emit drawingOperationCreated(operation);
        }

        // 铅笔绘图完成
        m_currentPath = nullptr;

        // 保存当前状态（用于撤销）
        saveState();
        // 通知内容已修改
        emit contentModified();
    }
}

// 在铅笔绘图的mouseMoveEvent中添加
void DrawingTool::drawPencil(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (!m_currentPath) {
            m_currentPath = new QGraphicsPathItem();
            QPainterPath path;
            path.moveTo(m_startPoint);
            m_currentPath->setPath(path);
            m_currentPath->setPen(m_pen);
            m_scene->addItem(m_currentPath);

            // 发送开始笔画消息
            if (m_isOnlineMode) {
                DrawingOperation operation;
                operation.opType = DOT_BeginStroke;
                operation.data = QVariantMap{
                    {"startX", m_startPoint.x()},
                    {"startY", m_startPoint.y()},
                    {"penColor", m_pen.color()},
                    {"penWidth", m_pen.width()}
                };
                emit drawingOperationCreated(operation);
            }
        }

        QPainterPath path = m_currentPath->path();
        QPointF currentPos = event->scenePos();
        path.lineTo(currentPos);
        m_currentPath->setPath(path);

        // 发送添加点消息
        if (m_isOnlineMode) {
            DrawingOperation operation;
            operation.opType = DOT_AddPoint;
            operation.data = QVariantMap{
                {"x", currentPos.x()},
                {"y", currentPos.y()}
            };
            emit drawingOperationCreated(operation);
        }
    } else {
        // 结束笔画
        if (m_currentPath && m_isOnlineMode) {
            DrawingOperation operation;
            operation.opType = DOT_EndStroke;
            operation.data = QVariantMap{
                {"path", QVariant::fromValue(m_currentPath->path())},
                {"penColor", m_pen.color()},
                {"penWidth", m_pen.width()}
            };
            emit drawingOperationCreated(operation);
        }
        m_currentPath = nullptr;
    }
}

// 直线绘图
void DrawingTool::drawLine(QGraphicsSceneMouseEvent *event)
{
    if (!m_isDrawing) return;

    if (m_tempItem) {
        QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem*>(m_tempItem);
        if (line) {
            line->setLine(QLineF(m_startPoint, m_endPoint));
            line->setPen(m_pen);  // 使用当前画笔
        }
    } else {
        QGraphicsLineItem *line = new QGraphicsLineItem(QLineF(m_startPoint, m_endPoint));
        line->setPen(m_pen);  // 使用当前画笔
        // 把修改的结果添加到布局中，从而达到绘制的效果
        m_scene->addItem(line);
        m_tempItem = line;
    }
}

// 矩形绘图
void DrawingTool::drawRectangle(QGraphicsSceneMouseEvent *event)
{
    if (!m_isDrawing) return;

    QRectF rect(m_startPoint, m_endPoint);
    rect = rect.normalized();

    if (m_tempItem) {
        QGraphicsRectItem *rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(m_tempItem);
        if (rectItem) {
            rectItem->setRect(rect);
            rectItem->setPen(m_pen);    // 使用当前画笔
            if(is_full){
                rectItem->setBrush(m_brush); // 使用当前画刷
            }
        }
    } else {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(rect);
        rectItem->setPen(m_pen);    // 使用当前画笔
        if(is_full){
            rectItem->setBrush(m_brush); // 使用当前画刷
        }
        // 把修改的结果添加到布局中，从而达到绘制的效果
        m_scene->addItem(rectItem);
        m_tempItem = rectItem;
    }
}

// 椭圆绘图
void DrawingTool::drawEllipse(QGraphicsSceneMouseEvent *event)
{
    if (!m_isDrawing) return;

    QRectF rect(m_startPoint, m_endPoint);
    // normalized() 方法的作用是 规范化矩形，确保矩形的宽度和高度都是正值
    rect = rect.normalized();

    if (m_tempItem) {
        QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(m_tempItem);
        if (ellipse) {
            ellipse->setRect(rect);
            ellipse->setPen(m_pen);    // 使用当前画笔
            // 对图形进行填充
            if(is_full){
                ellipse->setBrush(m_brush); // 使用当前画刷
            }
        }
    } else {
        QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(rect);
        ellipse->setPen(m_pen);    // 使用当前画笔
        if(is_full){
            ellipse->setBrush(m_brush); // 使用当前画刷
        }
        // 把修改的结果添加到布局中，从而达到绘制的效果
        m_scene->addItem(ellipse);
        m_tempItem = ellipse;
    }
}

// 在文本绘图中添加
void DrawingTool::drawText(QGraphicsSceneMouseEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        bool ok;
        QString text = QInputDialog::getText(nullptr, "编辑", "请输入文本:",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            // 保存添加文本前的状态
            saveState();

            QGraphicsTextItem *textItem = new QGraphicsTextItem(text);
            textItem->setPos(event->scenePos());
            // 设置字体大小
            QFont font = textItem->font(); // 获取当前字体
            font.setPointSize(font_weight); // 设置字体大小为12磅
            textItem->setFont(font); // 应用新字体

            textItem->setDefaultTextColor(m_textColor);
            m_scene->addItem(textItem);

            // 如果是网络模式，发送文本操作
            if (m_isOnlineMode) {
                DrawingOperation operation;
                operation.opType = DOT_AddText;
                operation.data = QVariantMap{
                    {"content", text},
                    {"x", event->scenePos().x()},
                    {"y", event->scenePos().y()},
                    {"fontSize", font_weight},
                    {"color", m_textColor}
                };
                emit drawingOperationCreated(operation);
            }

            // 文本添加完成，通知内容已修改
            emit contentModified();
        }
    }
}

// 替换原来的drawEraser方法
void DrawingTool::drawEraser(QGraphicsSceneMouseEvent *event)
{
    // 这个方法现在只用于显示橡皮擦的预览效果
    if (!m_tempItem) {
        // 创建橡皮擦预览图形
        qreal eraserSize = m_pen.width() + 10;
        // 橡皮擦删除图形
        QGraphicsEllipseItem *eraserPreview = new QGraphicsEllipseItem(
            -eraserSize/2, -eraserSize/2, eraserSize, eraserSize
        );

        // 设置橡皮擦预览样式
        QPen previewPen(Qt::gray, 1, Qt::DashLine);
        eraserPreview->setPen(previewPen);
        eraserPreview->setBrush(Qt::NoBrush);
        eraserPreview->setPos(event->scenePos());

        // 把修改的结果添加到布局中，从而达到绘制的效果
        m_scene->addItem(eraserPreview);
        m_tempItem = eraserPreview;
    } else {
        // 更新预览位置
        m_tempItem->setPos(event->scenePos());
    }
}

// 实现真正的擦除功能
void DrawingTool::eraseAtPosition(const QPointF &position)
{
    // 定义橡皮擦的大小（可以根据画笔宽度调整）
    qreal eraserSize = m_pen.width() + 10;

    // 创建橡皮擦区域（圆形）
    QRectF eraserArea(position.x() - eraserSize/2,
                      position.y() - eraserSize/2,
                      eraserSize,
                      eraserSize);

    // 查找与橡皮擦区域相交的所有图形项
    QList<QGraphicsItem*> itemsInArea = m_scene->items(eraserArea,
                                                        Qt::IntersectsItemShape,
                                                        Qt::DescendingOrder);

    // 擦除找到的项（除了橡皮擦临时项本身）
    foreach (QGraphicsItem* item, itemsInArea) {
        if (item != m_tempItem
            && item->data(Qt::UserRole).toString() != "grid") {
            // 保存被擦除的项（用于可能的撤销）
            if (!m_erasedItems.contains(item)) {
                m_erasedItems.append(item);
            }

            // 从场景中移除项
            m_scene->removeItem(item);
        }
    }
}

void DrawingTool::setPenColor(const QColor &color)
{
    m_pen.setColor(color);
}

void DrawingTool::setPenWidth(int width)
{
    m_pen.setWidth(width);
}

void DrawingTool::setBrushColor(const QColor &color)
{
    m_brush.setColor(color);
    m_brush.setStyle(Qt::SolidPattern);
}

// 设置文本颜色
void DrawingTool::setTextColor(const QColor &color)
{
    m_textColor = color;
}

// 获取当前颜色
QColor DrawingTool::currentPenColor() const { return m_pen.color(); }
QColor DrawingTool::currentBrushColor() const { return m_brush.color(); }
QColor DrawingTool::currentTextColor() const { return m_textColor; }


// 实现撤销
// void DrawingTool::undoErase()
// {
//     if (!m_erasedItems.isEmpty()) {
//         // 如果点击撤销按钮的话，需要将之前的给恢复
//         QGraphicsItem* item = m_erasedItems.takeLast();
//         m_scene->addItem(item);
//     }
// }

// 保存当前状态到撤销栈
void DrawingTool::saveState()
{
    if (m_scene) {
        // 收集当前场景中的所有项（不包含临时项）
        QList<QGraphicsItem*> currentState;
        // 保存当前的绘制结果
        foreach (QGraphicsItem* item, m_scene->items()) {
            // 不保存临时项（如橡皮擦预览、正在绘制的临时形状）
            if (item != m_tempItem && item != m_currentPath&&
                item->data(Qt::UserRole).toString() != "grid") {
                currentState.append(item);
            }
        }

        // 只有当状态确实发生变化时才保存
        if (!m_undoStack.isEmpty()) {
            QList<QGraphicsItem*> lastState = m_undoStack.last();
            // 如果当前场景状态和之前的一样就不用发生改变或者说不用保存
            if (lastState.size() == currentState.size()) {
                // 简单检查状态是否相同（可以更复杂的状态比较）
                bool sameState = true;
                // 遍历每一个状态是否发生变化
                for (int i = 0; i < lastState.size(); ++i) {
                    if (lastState[i] != currentState[i]) {
                        sameState = false;
                        break;
                    }
                }
                if (sameState) {
                    return; // 状态相同，不保存
                }
            }
        }

        m_undoStack.append(currentState);
        qDebug() << "保存状态，撤销栈大小:" << m_undoStack.size();

        // 限制栈大小，避免内存占用过多
        if (m_undoStack.size() > 50) { // 增加到50个状态
            QList<QGraphicsItem*> removedState = m_undoStack.takeFirst();
            // 清理被移除的状态中的items（如果需要）
            foreach (QGraphicsItem* item, removedState) {
                // 注意：这里不能删除item，因为它们可能还在场景中
            }
        }

        // 清空重做栈
        m_redoStack.clear();
    }
}

// 撤销
void DrawingTool::undo()
{
    if (m_undoStack.size() > 1) {
        try {
            // 保存当前状态到重做栈
            QList<QGraphicsItem*> currentState;
            foreach (QGraphicsItem* item, m_scene->items()) {
                if (item != m_tempItem && item != m_currentPath) {
                    currentState.append(item);
                }
            }
            m_redoStack.append(currentState);

            // 移除当前状态，获取上一个状态
            m_undoStack.removeLast();
            QList<QGraphicsItem*> previousState = m_undoStack.last();

            // 恢复上一个状态
            restoreState(previousState);

            // 如果是网络模式，发送撤销请求（包含操作信息）
            if (m_isOnlineMode) {
                // 获取最后操作的信息（需要扩展实现）
                QVariantMap undoData;
                undoData["operationType"] = "last"; // 或其他标识
                // undoData["operationId"] = generateOperationId();

                DrawingOperation operation;
                operation.opType = DOT_Undo;
                operation.data = undoData;

                emit undoRequestedWithData(operation);
            }

        } catch (const std::exception& e) {
            qWarning() << "撤销操作异常:" << e.what();
        }
    }
}

// 重做
void DrawingTool::redo()
{
    qDebug() << "重做操作，重做栈大小:" << m_redoStack.size();

    if (!m_redoStack.isEmpty()) {
        try {
            // 保存当前状态到撤销栈
            QList<QGraphicsItem*> currentState;
            foreach (QGraphicsItem* item, m_scene->items()) {
                if (item != m_tempItem && item != m_currentPath) {
                    currentState.append(item);
                }
            }
            m_undoStack.append(currentState);

            // 恢复重做状态
            QList<QGraphicsItem*> nextState = m_redoStack.takeLast();
            restoreState(nextState);

            qDebug() << "重做成功，当前重做栈大小:" << m_redoStack.size();

        } catch (const std::exception& e) {
            qWarning() << "重做操作异常:" << e.what();
        } catch (...) {
            qWarning() << "重做操作发生未知异常";
        }
    } else {
        qDebug() << "无法重做：重做栈为空";
    }

    // 如果是网络模式，发送重做请求
    if (m_isOnlineMode) {
        emit redoRequested();
    }
}

// 恢复场景状态（安全的实现）
void DrawingTool::restoreState(const QList<QGraphicsItem*>& state)
{
    if (m_scene) {
        // 清除当前场景（但不删除items，因为它们可能在其他地方被引用）
        QList<QGraphicsItem*> currentItems = m_scene->items();
        foreach (QGraphicsItem* item, currentItems) {
            // 只移除非临时项
            if (item != m_tempItem && item != m_currentPath
                &&item->data(Qt::UserRole).toString() != "grid") {
                m_scene->removeItem(item);
            }
        }

        // 添加新状态的items
        foreach (QGraphicsItem* item, state) {
            // 确保item没有被删除
            if (item && !m_scene->items().contains(item)) {
                m_scene->addItem(item);
            }
        }
    }
}

// 清除场景
void DrawingTool::clearScene()
{
    if (m_scene) {
        // 保存清除前的状态
        saveState();

        // 清除场景（但不删除items）
        QList<QGraphicsItem*> items = m_scene->items();
        foreach (QGraphicsItem* item, items) {
            // 跳过标记为"grid"的项
            if (item->data(Qt::UserRole).toString() != "grid") {
                m_scene->removeItem(item);
            }
            m_scene->removeItem(item);
        }

        emit sceneCleared();

        // 如果是网络模式，发送清除场景请求
        if (m_isOnlineMode) {
            emit clearSceneRequested();
        }

        // 重置临时变量
        m_tempItem = nullptr;
        m_isDrawing = false;
        m_isErasing = false;
        m_currentPath = nullptr;

        // 保存空白状态
        QList<QGraphicsItem*> emptyState;
        m_undoStack.append(emptyState);
    }
}

void DrawingTool::setOnlineMode(bool online)
{
    m_isOnlineMode = online;
}

bool DrawingTool::isOnlineMode() const
{
    return m_isOnlineMode;
}

DrawingOperationType DrawingTool::getCurrentOperationType() const
{
    switch (m_currentTool) {
        case Pencil:
            return DOT_BeginStroke;
        case Line:
            return DOT_DrawLine;
        case Rectangle:
            return DOT_DrawRectangle;
        case Ellipse:
            return DOT_DrawEllipse;
        case Text:
            return DOT_AddText;
        case Eraser:
            return DOT_Erase;
        default:
            return DOT_BeginStroke;
    }
}

QVariantMap DrawingTool::getCurrentOperationData(QGraphicsItem* finishedItem) const
{
    QVariantMap data;

    switch (m_currentTool) {
        case Pencil:
            // 铅笔操作需要保存路径点
            if (m_currentPath) {
                data["path"] =  QVariant::fromValue(m_currentPath->path());
                data["penColor"] = m_pen.color();
                data["penWidth"] = m_pen.width();
            }
            break;

        case Line:
            if (finishedItem) {
                QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem*>(finishedItem);
                if (line) {
                    data["x1"] = line->line().x1();
                    data["y1"] = line->line().y1();
                    data["x2"] = line->line().x2();
                    data["y2"] = line->line().y2();
                    data["penColor"] = m_pen.color();
                    data["penWidth"] = m_pen.width();
                }
            }
            break;

        case Rectangle:
            if (finishedItem) {
                QGraphicsRectItem *rect = qgraphicsitem_cast<QGraphicsRectItem*>(finishedItem);
                if (rect) {
                    std::cout<<"x = "<<rect->rect().x()<<" y = "<<rect->rect().y()<<std::endl;
                    std::cout<<"width = "<<rect->rect().width()<<" height = "<<rect->rect().height()<<std::endl;

                    data["x"] = rect->rect().x();
                    data["y"] = rect->rect().y();
                    data["width"] = rect->rect().width();
                    data["height"] = rect->rect().height();
                    data["penColor"] = m_pen.color();
                    data["penWidth"] = m_pen.width();
                    data["brushColor"] = m_brush.color();
                    data["isFilled"] = is_full;
                }
            }
            break;

        case Ellipse:
            if (finishedItem) {
                QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(finishedItem);
                if (ellipse) {
                    data["x"] = ellipse->rect().x();
                    data["y"] = ellipse->rect().y();
                    data["width"] = ellipse->rect().width();
                    data["height"] = ellipse->rect().height();
                    data["penColor"] = m_pen.color();
                    data["penWidth"] = m_pen.width();
                    data["brushColor"] = m_brush.color();
                    data["isFilled"] = is_full;
                }
            }
            break;

        case Text:
            // 文本操作在drawText中处理
            break;

        case Eraser:
            // 橡皮擦操作在eraseAtPosition中处理
            break;
    }

    return data;
}

// 具体的绘图操作动作
void DrawingTool::processNetworkOperation(const DrawingOperation &operation)
{
    // 在处理网络操作前保存状态
    if (operation.opType != DOT_Undo && operation.opType != DOT_Redo) {
        saveState();
    }
    switch (operation.opType) {
        case DOT_BeginStroke:
            // qDebug() << "开始笔画操作";
            break;
        case DOT_AddPoint:
            // qDebug() << "添加点操作";
            break;
        case DOT_EndStroke:
            // qDebug() << "结束笔画操作";
            if (operation.data.contains("path")) {
                // qDebug() << "包含路径数据";
                QVariant pathVar = operation.data["path"];
                if (pathVar.canConvert<QPainterPath>()) {
                    // qDebug() << "可以转换为QPainterPath";
                    QPainterPath path = pathVar.value<QPainterPath>();
                    // qDebug() << "路径元素数量:" << path.elementCount();
                }
            }
            drawNetworkPath(operation.data);
            break;
        case DOT_DrawLine:
            drawNetworkLine(operation.data);
            break;
        case DOT_DrawRectangle:
            std::cout<<"DOT DrawRectangle..."<<std::endl;
            drawNetworkRectangle(operation.data);
            break;
        case DOT_DrawEllipse:
            drawNetworkEllipse(operation.data);
            break;
        case DOT_AddText:
            addNetworkText(operation.data);
            break;
        case DOT_Erase:
            performNetworkErase(operation.data);
            break;
        case DOT_Undo:
            // 专门处理网络撤销
            processNetworkUndo(operation.data);
            break;
        case DOT_Redo:
            // 专门处理网络重做
            processNetworkRedo(operation.data);
            break;
        default:
            qDebug() << "未知的网络绘图操作类型:" << operation.opType;
        break;
    }
}


void DrawingTool::drawNetworkPath(const QVariantMap &data)
{
    std::cout << "drawNetworkPath - 开始处理网络路径" << std::endl;
    std::cout<<"drawNetworkPath"<<std::endl;
    std::cout<<"startX = "<<data["startX"].toDouble()<<" startY = "<<data["startY"].toDouble()<<std::endl;
    std::cout<<"x = "<<data["x"].toDouble()<<" y = "<<data["y"].toDouble()<<std::endl;
    // 检查是否包含路径数据
    if (data.contains("path")) {
        QVariant pathVariant = data["path"];
        if (pathVariant.canConvert<QPainterPath>()) {
            QPainterPath path = pathVariant.value<QPainterPath>();
            std::cout << "路径元素数量: " << path.elementCount() << std::endl;

            QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);

            // 设置画笔属性
            QPen pen;
            if (data.contains("penColor")) {
                pen.setColor(data["penColor"].value<QColor>());
            } else {
                pen.setColor(Qt::black);
            }

            if (data.contains("penWidth")) {
                pen.setWidth(data["penWidth"].toInt());
            } else {
                pen.setWidth(2);
            }

            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);

            pathItem->setPen(pen);
            m_scene->addItem(pathItem);

            m_currentNetworkPath = pathItem;

            std::cout << "成功添加路径到场景" << std::endl;
            return;
        }
    }

    // 备用方法：如果路径数据是以数组形式存储的
    if (data.contains("pathArray")) {
        QPainterPath path;
        QVariantList points = data["pathArray"].toList();

        if (!points.isEmpty()) {
            QVariant firstPoint = points.first();
            if (firstPoint.canConvert<QPointF>()) {
                path.moveTo(firstPoint.toPointF());

                for (int i = 1; i < points.size(); ++i) {
                    QVariant pointVar = points[i];
                    if (pointVar.canConvert<QPointF>()) {
                        path.lineTo(pointVar.toPointF());
                    }
                }

                QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
                QPen pen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                pathItem->setPen(pen);
                m_scene->addItem(pathItem);

                m_currentNetworkPath = pathItem;

                std::cout << "使用备用方法添加路径" << std::endl;
                return;
            }
        }
    }

    std::cout << "无法解析路径数据" << std::endl;
}

void DrawingTool::processNetworkUndo(const QVariantMap &data)
{
    if (m_undoStack.size() > 1) {
        // 保存当前状态到重做栈
        QList<QGraphicsItem*> currentState = getCurrentSceneState();
        m_redoStack.append(currentState);

        // 恢复到上一个状态
        m_undoStack.removeLast();
        QList<QGraphicsItem*> previousState = m_undoStack.last();
        restoreState(previousState);
    }
}

void DrawingTool::processNetworkRedo(const QVariantMap &data)
{
    qDebug() << "处理网络重做操作";

    if (m_redoStack.isEmpty()) {
        qDebug() << "重做栈为空，无法执行重做";
        return;
    }

    try {
        // 保存当前状态到撤销栈
        QList<QGraphicsItem*> currentState = getCurrentSceneState();
        m_undoStack.append(currentState);

        // 从重做栈获取下一个状态
        QList<QGraphicsItem*> nextState = m_redoStack.takeLast();

        // 恢复状态
        restoreState(nextState);

        qDebug() << "网络重做成功，重做栈大小:" << m_redoStack.size();

    } catch (const std::exception& e) {
        qWarning() << "网络重做操作异常:" << e.what();
    } catch (...) {
        qWarning() << "网络重做操作发生未知异常";
    }
}

QList<QGraphicsItem*> DrawingTool::getCurrentSceneState() const
{
    QList<QGraphicsItem*> currentState;

    if (!m_scene) return currentState;

    // 收集当前场景中的所有项（不包含临时项和网格）
    foreach (QGraphicsItem* item, m_scene->items()) {
        if (item != m_tempItem &&
            item != m_currentPath &&
            item != m_currentNetworkPath &&
            item->data(Qt::UserRole).toString() != "grid") {
            currentState.append(item);
        }
    }

    return currentState;
}

void DrawingTool::drawNetworkLine(const QVariantMap &data)
{
    std::cout<<"drawNetworkLine"<<std::endl;
    std::cout<<"x1 = "<<data["x1"].toDouble()<<" y1 = "<<data["y1"].toDouble()<<std::endl;
    std::cout<<"x2 = "<<data["x2"].toDouble()<<" y2 = "<<data["y2"].toDouble()<<std::endl;
    QGraphicsLineItem *line = new QGraphicsLineItem(
        data["x1"].toDouble(),
        data["y1"].toDouble(),
        data["x2"].toDouble(),
        data["y2"].toDouble()
        );
    line->setPen(QPen(
        data["penColor"].value<QColor>(),
        data["penWidth"].toInt()
        ));
    m_scene->addItem(line);
}

void DrawingTool::drawNetworkRectangle(const QVariantMap &data)
{
    std::cout<<"drawNetworkRectangle"<<std::endl;
    std::cout<<"x = "<<data["x"].toDouble()<<" y = "<<data["y"].toDouble()<<std::endl;
    std::cout<<"width = "<<data["width"].toDouble()<<" height = "<<data["height"].toDouble()<<std::endl;
    QGraphicsRectItem *rect = new QGraphicsRectItem(
        data["x"].toDouble(),
        data["y"].toDouble(),
        data["width"].toDouble(),
        data["height"].toDouble()
    );
    rect->setPen(QPen(
        data["penColor"].value<QColor>(),
        data["penWidth"].toInt()
    ));

    if (data["isFilled"].toBool()) {
        rect->setBrush(QBrush(data["brushColor"].value<QColor>()));
    }

    m_scene->addItem(rect);
}

void DrawingTool::drawNetworkEllipse(const QVariantMap &data)
{
    std::cout<<"drawNetworkEllipse"<<std::endl;
    std::cout<<"x = "<<data["x"].toDouble()<<" y = "<<data["y"].toDouble()<<std::endl;
    std::cout<<"width = "<<data["width"].toDouble()<<" height = "<<data["height"].toDouble()<<std::endl;
    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(
        data["x"].toDouble(),
        data["y"].toDouble(),
        data["width"].toDouble(),
        data["height"].toDouble()
    );
    ellipse->setPen(QPen(
        data["penColor"].value<QColor>(),
        data["penWidth"].toInt()
    ));

    if (data["isFilled"].toBool()) {
        ellipse->setBrush(QBrush(data["brushColor"].value<QColor>()));
    }

    m_scene->addItem(ellipse);
}

void DrawingTool::addNetworkText(const QVariantMap &data)
{
    QGraphicsTextItem *text = new QGraphicsTextItem(data["content"].toString());
    std::cout<<"addNetworkText"<<std::endl;
    std::cout<<"x = "<<data["x"].toDouble()<<" y = "<<data["y"].toDouble()<<std::endl;
    std::cout<<"text = "<<data["content"].toString().toStdString()<<std::endl;

    text->setPos(
        data["x"].toDouble(),
        data["y"].toDouble()
    );
    // 设置字体大小
    QFont font = text->font(); // 获取当前字体
    font.setPointSize(data["fontSize"].toInt()); // 设置字体大小为12磅
    text->setFont(font); // 应用新字体
    text->setDefaultTextColor(data["color"].value<QColor>());
    m_scene->addItem(text);
}

void DrawingTool::performNetworkErase(const QVariantMap &data)
{
    std::cout << "performNetworkErase" << std::endl;
    QRectF eraserArea(
        data["positionX"].toDouble() - data["eraserSize"].toDouble() / 2,
        data["positionY"].toDouble() - data["eraserSize"].toDouble() / 2,
        data["eraserSize"].toDouble(),
        data["eraserSize"].toDouble()
        );
    // 查找与指定矩形区域相交的所有图形项，比如有直线，椭圆，矩形，如果删除区域和矩形存在交集，那么“图形项”中就只有“矩形”
    QList<QGraphicsItem*> itemsInArea = m_scene->items(eraserArea, Qt::IntersectsItemShape);
    for (QGraphicsItem* item : itemsInArea) {
        if (item->data(Qt::UserRole).toString() == "grid") continue;

        // 对路径项进行精确碰撞检测
        if (QGraphicsPathItem* pathItem = qgraphicsitem_cast<QGraphicsPathItem*>(item)) {
            if (isPathIntersecting(pathItem, eraserArea)) {
                m_scene->removeItem(item);
            }
        } else if (item->shape().intersects(eraserArea)) {
            // 其他图形项的精确检测
            m_scene->removeItem(item);
        }
    }
}

bool DrawingTool::isPathIntersecting(QGraphicsPathItem* pathItem, const QRectF& eraserArea)
{
    QPainterPath path = pathItem->path();

    // 检查路径是否与橡皮擦区域相交
    if (path.intersects(eraserArea)) {
        return true;
    }

    // 更精确的检测：检查路径中的每个线段
    for (int i = 1; i < path.elementCount(); ++i) {
        QPointF p1(path.elementAt(i-1).x, path.elementAt(i-1).y);
        QPointF p2(path.elementAt(i).x, path.elementAt(i).y);

        // 检查线段是否与橡皮擦区域相交
        if (isLineIntersectingRect(p1, p2, eraserArea)) {
            return true;
        }
    }

    return false;
}

bool DrawingTool::isLineIntersectingRect(const QPointF& p1, const QPointF& p2, const QRectF& rect)
{
    // 使用Cohen-Sutherland算法检测线段与矩形相交
    // 这里简化实现，实际项目中需要完整算法
    return rect.contains(p1) || rect.contains(p2) ||
           rect.intersects(QRectF(p1, p2).normalized());
}
