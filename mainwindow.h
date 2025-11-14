/**
 * @file mainwindow.h
 * @brief 声明主窗口类，负责界面布局与播放控制。
 * @mainfunctions
 *   - handleStart
 *   - handleStop
 *   - handleStatusChanged
 *   - handleStatsUpdated
 *   - handleError
 *   - updateControlsForRunning
 * @mainclasses
 *   - MainWindow
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "playerstats.h"

class QLineEdit;
class QPushButton;
class QLabel;
class QSpinBox;
class VideoWidget;
class LiveStreamPlayer;

/**
 * @brief MainWindow 负责搭建 UI、连接信号槽并驱动拉流播放器。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化界面布局及信号连接。
     * @param parent 父级 QWidget。
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * @brief 析构函数，确保播放器安全停止。
     */
    ~MainWindow() override;

private slots:
    /**
     * @brief 处理开始按钮点击并启动播放。
     */
    void handleStart();

    /**
     * @brief 处理停止按钮点击并终止播放。
     */
    void handleStop();

    /**
     * @brief 当播放器状态变化时更新状态标签。
     * @param statusText 新的状态文本。
     */
    void handleStatusChanged(const QString& statusText);

    /**
     * @brief 接收实时统计数据并刷新显示。
     * @param stats 播放器统计信息。
     */
    void handleStatsUpdated(const PlayerStats& stats);

    /**
     * @brief 接收错误并提示用户。
     * @param message 错误描述。
     */
    void handleError(const QString& message);

private:
    /**
     * @brief 根据运行状态切换按钮可用性。
     * @param running 当前是否正在播放。
     */
    void updateControlsForRunning(bool running);

    LiveStreamPlayer* m_player = nullptr;
    VideoWidget* m_videoWidget = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_statsLabel = nullptr;

    // Settings controls
    QSpinBox* m_retrySpin = nullptr; // 最大重试次数
    QSpinBox* m_delaySpin = nullptr; // 重试间隔(ms)
};

#endif // MAINWINDOW_H
