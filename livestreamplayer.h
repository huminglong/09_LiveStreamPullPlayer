/**
 * @file livestreamplayer.h
 * @brief 定义 LiveStreamPlayer，负责拉流、解码与音视频队列管理。
 * @mainfunctions
 *   - start
 *   - stop
 *   - setMaxReconnectAttempts
 *   - setReconnectDelayMs
 *   - requestStop
 *   - demuxLoop
 *   - videoDecodeLoop
 *   - audioDecodeLoop
 *   - openStream
 *   - closeStream
 *   - setupAudioOutput
 *   - teardownAudioOutput
 * @mainclasses
 *   - LiveStreamPlayer
 */

#ifndef LIVESTREAMPLAYER_H
#define LIVESTREAMPLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QByteArray>
#include <QImage>
#include <QString>
class QTimer;
class QUrl;

#include <atomic>
#include <future>
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

/**
 * @brief LiveStreamPlayer 处理网络拉流、解码及音视频输出。
 */
class LiveStreamPlayer : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化队列与 FFmpeg 依赖。
     * @param parent Qt 父对象。
     */
    explicit LiveStreamPlayer(QObject* parent = nullptr);

    /**
     * @brief 析构函数，确保线程与资源被释放。
     */
    ~LiveStreamPlayer() override;

    /**
     * @brief 启动拉流流程并创建解复用/解码线程。
     * @param url 目标流地址。
     */
    void start(const QString& url);

    /**
     * @brief 停止播放并清理所有资源。
     */
    void stop();

    /**
     * @brief 查询播放器是否仍在运行。
     * @return true 表示正在运行。
     */
    bool isRunning() const;

    /**
     * @brief 设置最大重连次数。
     * @param attempts 最大尝试次数。
     */
    void setMaxReconnectAttempts(int attempts);

    /**
     * @brief 设置重连延迟（毫秒）。
     * @param delayMs 等待时长。
     */
    void setReconnectDelayMs(int delayMs);

signals:
    /**
     * @brief 当有新的视频帧准备好时发射。
     * @param frame 已转换的 QImage。
     */
    void frameReady(const QImage& frame);

    /**
     * @brief 播放状态变化时通知 UI。
     * @param statusText 状态描述。
     */
    void statusChanged(const QString& statusText);

    /**
     * @brief 推送统计信息供 UI 展示。
     * @param stats 当前统计数据。
     */
    void statsUpdated(const PlayerStats& stats);

    /**
     * @brief 当出现错误时发射，供 UI 提示。
     * @param message 错误文本。
     */
    void errorOccurred(const QString& message);

public slots:
    /**
     * @brief 请求停止播放，触发清理流程。
     */
    void requestStop();

private:
    /**
     * @brief 解复用循环，负责读取网络数据并入队。
     * @param url 当前拉流地址。
     */
    void demuxLoop(QString url);

    /**
     * @brief 视频解码线程主循环。
     */
    void videoDecodeLoop();

    /**
     * @brief 音频解码线程主循环。
     */
    void audioDecodeLoop();

    /**
     * @brief 打开流并准备解码上下文。
     * @param url 拉流地址。
     * @return 成功返回 true。
     */
    bool openStream(const QString& url);

    /**
     * @brief 关闭流并释放 FFmpeg 上下文。
     */
    void closeStream();

    /**
     * @brief 清空音视频包队列。
     */
    void clearQueues();

    /**
     * @brief 重置播放器内部状态。
     */
    void resetState();

    /**
     * @brief 初始化音频输出设备。
     * @param sampleRate 目标采样率。
     * @param channels 通道数。
     */
    void setupAudioOutput(int sampleRate, int channels);

    /**
     * @brief 停止音频输出并释放设备。
     */
    void teardownAudioOutput();

    /**
     * @brief 将解码后的音频数据写入 QAudioOutput。
     * @param samples PCM 数据。
     */
    void emitAudioSamples(QByteArray samples);

    /**
     * @brief 刷新统计数据并发射信号。
     */
    void updateStats();

    /**
     * @brief 将 FFmpeg 错误码转换为可读字符串。
     * @param errorCode FFmpeg 返回值。
     * @return 对应错误文本。
     */
    static QString ffmpegErrorString(int errorCode);

    /**
     * @brief FFmpeg I/O 中断回调，用于终止阻塞调用。
     * @param opaque 回调上下文。
     * @return 1 表示中断。
     */
    static int interruptCallback(void* opaque);

    /**
     * @brief 清理输入 URL 里的无效参数。
     * @param url 原始地址。
     * @return 规范化后的地址。
     */
    static QString sanitizeInputUrl(const QString& url);

    /**
     * @brief 提取并小写化 URL 协议名。
     * @param url 原始地址。
     * @return 小写协议名。
     */
    static QString urlSchemeLower(const QString& url);

    /**
     * @brief 实际的阻塞停止实现，由异步封装调用。
     */
    void stopInternal();

    /**
     * @brief 等待异步停止任务完成，避免资源竞争。
     */
    void waitForShutdownCompletion();

    std::thread m_demuxThread;
    std::thread m_videoThread;
    std::thread m_audioThread;

    std::atomic_bool m_running{ false };
    std::atomic_bool m_stopRequested{ false };

    PacketQueue m_videoQueue;
    PacketQueue m_audioQueue;

    std::mutex m_contextMutex;
    AVFormatContext* m_formatCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVCodecContext* m_audioCodecCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    SwrContext* m_swrCtx = nullptr;
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    double m_videoFrameDurationMs = 0.0;
    double m_audioFrameDurationMs = 0.0;

    std::atomic<int> m_targetSampleRate{ 0 };
    std::atomic<int> m_targetChannels{ 0 };

    QAudioOutput* m_audioOutput = nullptr;
    QIODevice* m_audioDevice = nullptr;
    QTimer* m_statsTimer = nullptr;

    std::atomic<double> m_bitrateKbps{ 0.0 };
    QString m_currentUrl;

    // Reconnect configuration
    std::atomic<int> m_maxReconnectAttempts{ 5 };
    std::atomic<int> m_reconnectDelayMs{ 2000 };

    std::shared_future<void> m_shutdownFuture;
    mutable std::mutex m_shutdownMutex;
    std::atomic_bool m_stopInProgress{ false };
};

#endif // LIVESTREAMPLAYER_H
