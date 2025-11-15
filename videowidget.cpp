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
#include <QPainterPath>
#include <QMutexLocker>
#include <QFont>

 /**
  * @brief 构造函数，设定背景和最小尺寸。
  * @param parent 父级 QWidget。
  */
VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent) {
    setAutoFillBackground(true);
    setMinimumSize(320, 240);

    // 设置现代化样式：圆角边框和阴影效果
    setStyleSheet(
        "VideoWidget {"
        "   background-color: #000000;"
        "   border: 3px solid #4CAF50;"
        "   border-radius: 10px;"
        "}"
    );
}

/**
 * @brief 更新当前帧并触发重绘。
 * @param frame 最新图像。
 */
void VideoWidget::updateFrame(const QImage& frame) {
    {
        QMutexLocker locker(&m_mutex);
        m_frame = frame;
    }
    update();
}

/**
 * @brief 清除当前帧并触发重绘。
 */
void VideoWidget::clearFrame() {
    {
        QMutexLocker locker(&m_mutex);
        m_frame = QImage(); // 清空图像
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

    // 启用抗锯齿以获得更平滑的渲染效果
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 创建圆角矩形裁剪区域
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    painter.setClipPath(path);

    painter.fillRect(rect(), Qt::black);

    QImage frameCopy;
    {
        QMutexLocker locker(&m_mutex);
        frameCopy = m_frame;
    }

    if (frameCopy.isNull()) {
        // 如果没有视频帧，显示提示文字
        painter.setPen(QColor(150, 150, 150));
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("🎬 等待视频流..."));
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
