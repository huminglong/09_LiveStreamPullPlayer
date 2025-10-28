#ifndef LIVESTREAMPLAYER_H
#define LIVESTREAMPLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QByteArray>
#include <QImage>
#include <QString>
class QUrl;

#include <atomic>
#include <mutex>
#include <thread>

#include "packetqueue.h"
#include "playerstats.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class LiveStreamPlayer : public QObject
{
    Q_OBJECT
public:
    explicit LiveStreamPlayer(QObject *parent = nullptr);
    ~LiveStreamPlayer() override;

    void start(const QString &url);
    void stop();
    bool isRunning() const;

    // Configuration
    void setMaxReconnectAttempts(int attempts);
    void setReconnectDelayMs(int delayMs);

signals:
    void frameReady(const QImage &frame);
    void statusChanged(const QString &statusText);
    void statsUpdated(const PlayerStats &stats);
    void errorOccurred(const QString &message);

public slots:
    void requestStop();

private:
    void demuxLoop(QString url);
    void videoDecodeLoop();
    void audioDecodeLoop();

    bool openStream(const QString &url);
    void closeStream();
    void clearQueues();
    void resetState();

    void setupAudioOutput(int sampleRate, int channels);
    void teardownAudioOutput();
    void emitAudioSamples(QByteArray samples);
    void updateStats();

    static QString ffmpegErrorString(int errorCode);
    static int interruptCallback(void *opaque);

    // Helpers
    static QString sanitizeInputUrl(const QString &url);
    static QString urlSchemeLower(const QString &url);

    std::thread m_demuxThread;
    std::thread m_videoThread;
    std::thread m_audioThread;

    std::atomic_bool m_running{false};
    std::atomic_bool m_stopRequested{false};

    PacketQueue m_videoQueue;
    PacketQueue m_audioQueue;

    std::mutex m_contextMutex;
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    double m_videoFrameDurationMs = 0.0;
    double m_audioFrameDurationMs = 0.0;

    std::atomic<int> m_targetSampleRate{0};
    std::atomic<int> m_targetChannels{0};

    QAudioOutput *m_audioOutput = nullptr;
    QIODevice *m_audioDevice = nullptr;

    std::atomic<double> m_bitrateKbps{0.0};
    QString m_currentUrl;

    // Reconnect configuration
    std::atomic<int> m_maxReconnectAttempts{5};
    std::atomic<int> m_reconnectDelayMs{2000};
};

#endif // LIVESTREAMPLAYER_H
