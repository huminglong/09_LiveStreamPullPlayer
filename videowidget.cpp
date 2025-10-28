#include "videowidget.h"

#include <QPainter>
#include <QMutexLocker>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setMinimumSize(320, 240);
}

void VideoWidget::updateFrame(const QImage &frame)
{
    {
        QMutexLocker locker(&m_mutex);
        m_frame = frame.copy();
    }
    update();
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QImage frameCopy;
    {
        QMutexLocker locker(&m_mutex);
        frameCopy = m_frame;
    }

    if (frameCopy.isNull())
    {
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

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
