/**
 * @file mainwindow.cpp
 * @brief 实现主窗口界面逻辑与信号槽处理。
 * @mainfunctions
 *   - MainWindow::handleStart
 *   - MainWindow::handleStop
 *   - MainWindow::handleStatusChanged
 *   - MainWindow::handleStatsUpdated
 *   - MainWindow::handleError
 *   - MainWindow::updateControlsForRunning
 * @mainclasses
 *   - MainWindow
 */

#include "mainwindow.h"

#include "livestreamplayer.h"
#include "videowidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

 /**
  * @brief 构造主窗口并设置所有界面元素。
  * @param parent 父级 QWidget。
  */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    m_player = new LiveStreamPlayer(this);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);

    auto* urlLayout = new QHBoxLayout();
    auto* urlLabel = new QLabel(QStringLiteral("Stream URL:"), central);
    m_urlEdit = new QLineEdit(central);
    m_urlEdit->setPlaceholderText(QStringLiteral("rtsp:// or rtmp://"));
    urlLayout->addWidget(urlLabel);
    urlLayout->addWidget(m_urlEdit);

    // Settings row: max retries and retry delay
    auto* settingsLayout = new QHBoxLayout();
    auto* retryLabel = new QLabel(QStringLiteral("Max retries:"), central);
    m_retrySpin = new QSpinBox(central);
    m_retrySpin->setRange(0, 100);
    m_retrySpin->setValue(5);
    auto* delayLabel = new QLabel(QStringLiteral("Retry delay (ms):"), central);
    m_delaySpin = new QSpinBox(central);
    m_delaySpin->setRange(0, 60000);
    m_delaySpin->setSingleStep(100);
    m_delaySpin->setValue(2000);
    settingsLayout->addWidget(retryLabel);
    settingsLayout->addWidget(m_retrySpin);
    settingsLayout->addSpacing(16);
    settingsLayout->addWidget(delayLabel);
    settingsLayout->addWidget(m_delaySpin);
    settingsLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(QStringLiteral("Connect"), central);
    m_stopButton = new QPushButton(QStringLiteral("Disconnect"), central);
    m_stopButton->setEnabled(false);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    m_statusLabel = new QLabel(QStringLiteral("Idle"), central);
    m_statsLabel = new QLabel(QStringLiteral("Video queue: 0 | Audio queue: 0 | Bitrate: 0.0 kbps | Jitter: 0.0 ms | Dropped: 0"), central);

    m_videoWidget = new VideoWidget(central);

    mainLayout->addLayout(urlLayout);
    mainLayout->addLayout(settingsLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_statsLabel);
    mainLayout->addWidget(m_videoWidget, 1);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::handleStart);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::handleStop);

    connect(m_player, &LiveStreamPlayer::frameReady, m_videoWidget, &VideoWidget::updateFrame, Qt::QueuedConnection);
    connect(m_player, &LiveStreamPlayer::statusChanged, this, &MainWindow::handleStatusChanged);
    connect(m_player, &LiveStreamPlayer::statsUpdated, this, &MainWindow::handleStatsUpdated);
    connect(m_player, &LiveStreamPlayer::errorOccurred, this, &MainWindow::handleError);

    setWindowTitle(QStringLiteral("Live Stream Pull Player"));
    resize(960, 640);
}

/**
 * @brief 析构函数，确保播放器在线程退出后释放。
 */
MainWindow::~MainWindow() {
    if (m_player) {
        m_player->stop();
    }
}

/**
 * @brief 校验 URL 并启动播放器，应用重试设置。
 */
void MainWindow::handleStart() {
    if (!m_player) {
        return;
    }

    const QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Missing URL"), QStringLiteral("Please enter a valid RTSP or RTMP address."));
        return;
    }

    // Apply settings before starting
    if (m_retrySpin)
        m_player->setMaxReconnectAttempts(m_retrySpin->value());
    if (m_delaySpin)
        m_player->setReconnectDelayMs(m_delaySpin->value());

    updateControlsForRunning(true);
    m_player->start(url);
}

/**
 * @brief 停止播放器并重置按钮状态。
 */
void MainWindow::handleStop() {
    if (!m_player) {
        return;
    }

    m_player->stop();
    updateControlsForRunning(false);
    m_statusLabel->setText(QStringLiteral("Stopped"));
}

/**
 * @brief 更新状态标签并在播放时锁定开始按钮。
 * @param statusText 播放器状态文本。
 */
void MainWindow::handleStatusChanged(const QString& statusText) {
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->setText(statusText);
    if (statusText.compare(QStringLiteral("Playing"), Qt::CaseInsensitive) == 0) {
        updateControlsForRunning(true);
    }
}

/**
 * @brief 将统计数据格式化后展示。
 * @param stats 播放器实时统计。
 */
void MainWindow::handleStatsUpdated(const PlayerStats& stats) {
    if (!m_statsLabel) {
        return;
    }

    m_statsLabel->setText(QStringLiteral("Video queue: %1 | Audio queue: %2 | Bitrate: %3 kbps | Jitter: %4 ms | Dropped: %5")
        .arg(stats.videoQueueSize)
        .arg(stats.audioQueueSize)
        .arg(QString::number(stats.incomingBitrateKbps, 'f', 1))
        .arg(QString::number(stats.jitterBufferMs, 'f', 1))
        .arg(stats.droppedVideoFrames));
}

/**
 * @brief 弹出错误对话框并恢复按钮状态。
 * @param message 错误描述。
 */
void MainWindow::handleError(const QString& message) {
    if (message.isEmpty()) {
        return;
    }

    QMessageBox::warning(this, QStringLiteral("Playback Error"), message);
    updateControlsForRunning(false);
}

/**
 * @brief 根据运行状态切换开始/停止按钮可用性。
 * @param running 是否正在播放。
 */
void MainWindow::updateControlsForRunning(bool running) {
    if (!m_startButton || !m_stopButton) {
        return;
    }

    m_startButton->setEnabled(!running);
    m_stopButton->setEnabled(running);
}
