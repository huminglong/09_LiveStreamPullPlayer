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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleStart();
    void handleStop();
    void handleStatusChanged(const QString &statusText);
    void handleStatsUpdated(const PlayerStats &stats);
    void handleError(const QString &message);

private:
    void updateControlsForRunning(bool running);

    LiveStreamPlayer *m_player = nullptr;
    VideoWidget *m_videoWidget = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_statsLabel = nullptr;

    // Settings controls
    QSpinBox *m_retrySpin = nullptr; // 最大重试次数
    QSpinBox *m_delaySpin = nullptr; // 重试间隔(ms)
};

#endif // MAINWINDOW_H
