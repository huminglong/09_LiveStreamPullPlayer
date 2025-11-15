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
#include <QIcon>
#include <QPixmap>

 /**
  * @brief 构造主窗口并设置所有界面元素。
  * @param parent 父级 QWidget。
  */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    m_player = new LiveStreamPlayer(this);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    // 设置现代化样式表
    setStyleSheet(
        "QMainWindow {"
        "   background-color: #f5f5f5;"
        "}"
        "QWidget#centralWidget {"
        "   background-color: #f5f5f5;"
        "}"
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #45a049);"
        "   color: white;"
        "   border: none;"
        "   border-radius: 6px;"
        "   padding: 8px 20px;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   min-width: 100px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5CBF60, stop:1 #55b059);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3d8b40, stop:1 #368a39);"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #888888;"
        "}"
        "QPushButton#stopButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f44336, stop:1 #da190b);"
        "}"
        "QPushButton#stopButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff5346, stop:1 #ea291b);"
        "}"
        "QPushButton#stopButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d32f2f, stop:1 #c62828);"
        "}"
        "QLineEdit {"
        "   background-color: white;"
        "   border: 2px solid #ddd;"
        "   border-radius: 6px;"
        "   padding: 6px 12px;"
        "   font-size: 13px;"
        "   selection-background-color: #4CAF50;"
        "}"
        "QLineEdit:focus {"
        "   border: 2px solid #4CAF50;"
        "}"
        "QSpinBox {"
        "   background-color: white;"
        "   border: 2px solid #ddd;"
        "   border-radius: 6px;"
        "   padding: 4px 8px;"
        "   font-size: 13px;"
        "   min-width: 80px;"
        "}"
        "QSpinBox:focus {"
        "   border: 2px solid #4CAF50;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "   background-color: #e0e0e0;"
        "   border-radius: 3px;"
        "   width: 18px;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "   background-color: #d0d0d0;"
        "}"
        "QLabel {"
        "   color: #333;"
        "   font-size: 13px;"
        "}"
        "QLabel#titleLabel {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: #2c3e50;"
        "}"
        "QLabel#statusLabel {"
        "   background-color: white;"
        "   border: 2px solid #ddd;"
        "   border-radius: 6px;"
        "   padding: 6px 12px;"
        "   font-weight: bold;"
        "   color: #2196F3;"
        "}"
        "QLabel#statsLabel {"
        "   background-color: white;"
        "   border: 2px solid #ddd;"
        "   border-radius: 6px;"
        "   padding: 6px 12px;"
        "   font-size: 12px;"
        "   color: #666;"
        "}"
    );
    central->setObjectName("centralWidget");

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(20, 20, 20, 20); // 设置外边距
    mainLayout->setSpacing(12); // 设置组件间距

    auto* urlLayout = new QHBoxLayout();
    urlLayout->setSpacing(10);
    auto* urlLabel = new QLabel(central);
    urlLabel->setPixmap(QPixmap(":/icons/icons/stream.svg").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* urlTextLabel = new QLabel(QStringLiteral("流地址:"), central);
    urlTextLabel->setObjectName("titleLabel");
    m_urlEdit = new QLineEdit(central);
    m_urlEdit->setPlaceholderText(QStringLiteral("输入 rtsp:// 或 rtmp:// 地址"));
    urlLayout->addWidget(urlLabel);
    urlLayout->addWidget(urlTextLabel);
    urlLayout->addWidget(m_urlEdit, 1); // 让输入框占据剩余空间

    // Settings row: max retries and retry delay
    auto* settingsLayout = new QHBoxLayout();
    settingsLayout->setSpacing(10);
    auto* retryIconLabel = new QLabel(central);
    retryIconLabel->setPixmap(QPixmap(":/icons/icons/retry.svg").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* retryLabel = new QLabel(QStringLiteral("最大重试:"), central);
    retryLabel->setObjectName("titleLabel");
    m_retrySpin = new QSpinBox(central);
    m_retrySpin->setRange(0, 100);
    m_retrySpin->setValue(5);
    auto* delayIconLabel = new QLabel(central);
    delayIconLabel->setPixmap(QPixmap(":/icons/icons/timer.svg").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* delayLabel = new QLabel(QStringLiteral("重试间隔(ms):"), central);
    delayLabel->setObjectName("titleLabel");
    m_delaySpin = new QSpinBox(central);
    m_delaySpin->setRange(0, 60000);
    m_delaySpin->setSingleStep(100);
    m_delaySpin->setValue(2000);
    settingsLayout->addWidget(retryIconLabel);
    settingsLayout->addWidget(retryLabel);
    settingsLayout->addWidget(m_retrySpin);
    settingsLayout->addSpacing(30); // 增加间距
    settingsLayout->addWidget(delayIconLabel);
    settingsLayout->addWidget(delayLabel);
    settingsLayout->addWidget(m_delaySpin);
    settingsLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    m_startButton = new QPushButton(QIcon(":/icons/icons/play.svg"), QStringLiteral(" 开始播放"), central);
    m_startButton->setObjectName("startButton");
    m_startButton->setCursor(Qt::PointingHandCursor); // 设置鼠标悬停样式
    m_startButton->setIconSize(QSize(20, 20));
    m_stopButton = new QPushButton(QIcon(":/icons/icons/stop.svg"), QStringLiteral(" 停止播放"), central);
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setIconSize(QSize(20, 20));
    m_stopButton->setEnabled(false);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    m_statusLabel = new QLabel(QStringLiteral("空闲中"), central);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter); // 居中对齐
    m_statsLabel = new QLabel(QStringLiteral("视频队列: 0 | 音频队列: 0 | 码率: 0.0 kbps | 抖动: 0.0 ms | 丢帧: 0"), central);
    m_statsLabel->setObjectName("statsLabel");
    m_statsLabel->setAlignment(Qt::AlignCenter);

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

    // 设置窗口标题和属性
    setWindowTitle(QStringLiteral("直播流播放器"));
    setWindowIcon(QIcon(":/icons/icons/app_icon.svg")); // 设置窗口图标
    setMinimumSize(800, 600); // 设置最小窗口尺寸
    resize(1080, 720); // 设置默认窗口尺寸

    // 设置窗口标志，避免图标重复显示
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);    // 窗口居中显示
    if (QWidget* parentWidget = qobject_cast<QWidget*>(parent)) {
        move(parentWidget->geometry().center() - rect().center());
    }
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
        QMessageBox::warning(this, QStringLiteral("缺少URL"), QStringLiteral("请输入有效的 RTSP 或 RTMP 地址"));
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

    // 清除视频画面
    if (m_videoWidget) {
        m_videoWidget->clearFrame();
    }

    updateControlsForRunning(false);
    m_statusLabel->setText(QStringLiteral("已停止"));
    m_statusLabel->setStyleSheet("background-color: white; border: 2px solid #ddd; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #F44336;");
}

/**
 * @brief 更新状态标签并在播放时锁定开始按钮。
 * @param statusText 播放器状态文本。
 */
void MainWindow::handleStatusChanged(const QString& statusText) {
    if (!m_statusLabel) {
        return;
    }

    // 根据状态设置文本和颜色
    QString displayText;
    QString colorStyle;
    if (statusText.contains(QStringLiteral("Playing"), Qt::CaseInsensitive)) {
        displayText = QStringLiteral("播放中");
        colorStyle = "color: #4CAF50; font-weight: bold;";
        updateControlsForRunning(true);
    }
    else if (statusText.contains(QStringLiteral("Connecting"), Qt::CaseInsensitive)) {
        displayText = QStringLiteral("连接中...");
        colorStyle = "color: #FF9800; font-weight: bold;";
    }
    else if (statusText.contains(QStringLiteral("Stopped"), Qt::CaseInsensitive)) {
        displayText = QStringLiteral("已停止");
        colorStyle = "color: #F44336; font-weight: bold;";
    }
    else if (statusText.contains(QStringLiteral("Error"), Qt::CaseInsensitive)) {
        displayText = QStringLiteral("错误");
        colorStyle = "color: #F44336; font-weight: bold;";
    }
    else if (statusText.contains(QStringLiteral("Idle"), Qt::CaseInsensitive)) {
        displayText = QStringLiteral("空闲中");
        colorStyle = "color: #2196F3; font-weight: bold;";
    }
    else {
        displayText = statusText;
        colorStyle = "color: #2196F3; font-weight: bold;";
    }

    m_statusLabel->setText(displayText);
    // 设置完整的内联样式，包含基础样式和状态颜色
    m_statusLabel->setStyleSheet("background-color: white; border: 2px solid #ddd; border-radius: 6px; padding: 6px 12px; font-weight: bold; " + colorStyle);
}

/**
 * @brief 将统计数据格式化后展示。
 * @param stats 播放器实时统计。
 */
void MainWindow::handleStatsUpdated(const PlayerStats& stats) {
    if (!m_statsLabel) {
        return;
    }

    m_statsLabel->setText(QStringLiteral("视频队列: %1 | 音频队列: %2 | 码率: %3 kbps | 抖动: %4 ms | 丢帧: %5")
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

    QMessageBox::warning(this, QStringLiteral("播放错误"), message);
    updateControlsForRunning(false);
    m_statusLabel->setText(QStringLiteral("错误"));
    m_statusLabel->setStyleSheet("background-color: white; border: 2px solid #ddd; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #F44336;");
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
