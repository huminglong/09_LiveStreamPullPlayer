/**
 * @file videowidget.cpp
 * @brief 实现视频显示控件的绘制与帧更新逻辑。
 * @mainfunctions
 *   - VideoWidget::updateFrame
 *   - VideoWidget::paintEvent
 *   - VideoWidget::resizeEvent
 * @mainclasses
 *   - VideoWidget
 */

#include "videowidget.h"

#include <QPainter>
#include <QMutexLocker>

 /**
  * @brief 构造函数，设定背景和最小尺寸。
  * @param parent 父级 QWidget。
  */
VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent) {
    setAutoFillBackground(true);
    setMinimumSize(320, 240);
}

/**
 * @brief 更新当前帧并触发重绘。
 * @param frame 最新图像。
 */
void VideoWidget::updateFrame(const QImage& frame) {
    {
        QMutexLocker locker(&m_mutex);
        m_frame = frame.copy();
    }
    update();
}

/**
 * @brief 以等比例缩放方式绘制当前帧。
 * @param event Qt 绘制事件。
 */
void VideoWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QImage frameCopy;
    {
        QMutexLocker locker(&m_mutex);
        frameCopy = m_frame;
    }

    if (frameCopy.isNull()) {
        return;
    }

    const QSize imageSize = frameCopy.size();
    const QSize widgetSize = size();
    QSize drawSize = imageSize;
    drawSize.scale(widgetSize, Qt::KeepAspectRatio);

    const int x = (widgetSize.width() - drawSize.width()) / 2;
    const int y = (widgetSize.height() - drawSize.height()) / 2;

    painter.drawImage(QRect(QPoint(x, y), drawSize), frameCopy);
}

/**
 * @brief 响应尺寸变化并请求重绘。
 * @param event Qt 调整事件。
 */
void VideoWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}
