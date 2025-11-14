/**
 * @file videowidget.h
 * @brief 声明用于显示视频帧的 QWidget 子类。
 * @mainfunctions
 *   - updateFrame
 *   - paintEvent
 *   - resizeEvent
 * @mainclasses
 *   - VideoWidget
 */

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QImage>
#include <QMutex>
#include <QWidget>

 /**
  * @brief VideoWidget 负责接收图像并在界面中绘制。
  */
class VideoWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数，设置默认尺寸与背景。
     * @param parent 父级 QWidget。
     */
    explicit VideoWidget(QWidget* parent = nullptr);

public slots:
    /**
     * @brief 更新最新帧并触发重绘。
     * @param frame 输入图像。
     */
    void updateFrame(const QImage& frame);

protected:
    /**
     * @brief 绘制当前帧，保持纵横比。
     * @param event Qt 绘制事件。
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief 处理窗口大小变化以重新绘制内容。
     * @param event Qt 调整事件。
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage m_frame;
    QMutex m_mutex;
};

#endif // VIDEOWIDGET_H
