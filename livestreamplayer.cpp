#include "livestreamplayer.h"

#include <QAudioDeviceInfo>
#include <QMetaObject>
#include <QThread>
#include <QtGlobal>

#include <algorithm>
#include <chrono>
#include <vector>
#include <type_traits>
#include <utility>
// Qt URL helpers
#include <QUrl>
#include <QUrlQuery>

// FFmpeg version info for compatibility helpers
extern "C"
{
#include <libavutil/version.h>
}

namespace
{
    constexpr int kReconnectDelayMs = 2000;          // default
    constexpr int kDemuxTimeoutUs = 5 * 1000 * 1000; // 5 seconds
    constexpr int kQueueMaxPacketsVideo = 90;
    constexpr int kQueueMaxPacketsAudio = 180;
    constexpr int kMaxReconnectAttempts = 5; // default 最大重试次数
}

// Helper to make av_channel_layout_default usable across FFmpeg versions
// Some FFmpeg versions declare av_channel_layout_default as void, others return int.
// This wrapper normalizes the behavior to return 0 on success, negative on failure when available.
// SFINAE-based detection: provide two overloads depending on whether
// av_channel_layout_default returns int or not. This avoids if constexpr issues
// with some IntelliSense/type-checkers while keeping full compatibility.
namespace
{
    // Determine the function pointer type of av_channel_layout_default.
    using av_cl_default_ptr_t = decltype(&av_channel_layout_default);

    template <typename Ptr = av_cl_default_ptr_t>
    auto ff_channel_layout_default_impl_int(AVChannelLayout *layout, int nb_channels)
        -> std::enable_if_t<std::is_same_v<Ptr, int (*)(AVChannelLayout *, int)>, int>
    {
        return av_channel_layout_default(layout, nb_channels);
    }

    template <typename Ptr = av_cl_default_ptr_t>
    auto ff_channel_layout_default_impl_void(AVChannelLayout *layout, int nb_channels)
        -> std::enable_if_t<!std::is_same_v<Ptr, int (*)(AVChannelLayout *, int)>, int>
    {
        av_channel_layout_default(layout, nb_channels);
        return 0;
    }
}

static inline int ff_channel_layout_default_compat(AVChannelLayout *layout, int nb_channels)
{
    // Call the underlying function and return 0. In FFmpeg versions where
    // av_channel_layout_default returns an int, we ignore it here for
    // compatibility with different toolchains and static analyzers.
    av_channel_layout_default(layout, nb_channels);
    return 0;
}

LiveStreamPlayer::LiveStreamPlayer(QObject *parent)
    : QObject(parent),
      m_videoQueue(kQueueMaxPacketsVideo),
      m_audioQueue(kQueueMaxPacketsAudio)
{
    qRegisterMetaType<PlayerStats>("PlayerStats");

    static std::once_flag initFlag;
    std::call_once(initFlag, []()
                   { avformat_network_init(); });
}

LiveStreamPlayer::~LiveStreamPlayer()
{
    stop();
}

void LiveStreamPlayer::start(const QString &url)
{
    if (url.isEmpty())
    {
        emit errorOccurred(QStringLiteral("Stream URL is empty."));
        return;
    }

    stop();

    m_currentUrl = sanitizeInputUrl(url);
    m_videoQueue.clear();
    m_audioQueue.clear();
    m_videoQueue.open();
    m_audioQueue.open();
    m_stopRequested.store(false);
    m_running.store(true);
    m_bitrateKbps.store(0.0, std::memory_order_release);

    emit statusChanged(QStringLiteral("Connecting"));
    updateStats();

    m_demuxThread = std::thread(&LiveStreamPlayer::demuxLoop, this, m_currentUrl);
    m_videoThread = std::thread(&LiveStreamPlayer::videoDecodeLoop, this);
    m_audioThread = std::thread(&LiveStreamPlayer::audioDecodeLoop, this);
}

void LiveStreamPlayer::stop()
{
    // Always perform full shutdown/cleanup even if m_running is already false
    m_running.store(false, std::memory_order_release);
    m_stopRequested.store(true, std::memory_order_release);

    m_videoQueue.close();
    m_audioQueue.close();

    if (m_demuxThread.joinable())
    {
        m_demuxThread.join();
    }
    if (m_videoThread.joinable())
    {
        m_videoThread.join();
    }
    if (m_audioThread.joinable())
    {
        m_audioThread.join();
    }

    clearQueues();
    closeStream();

    m_bitrateKbps.store(0.0, std::memory_order_release);

    updateStats();

    if (thread() == QThread::currentThread())
    {
        teardownAudioOutput();
    }
    else
    {
        QMetaObject::invokeMethod(this, &LiveStreamPlayer::teardownAudioOutput,
                                  Qt::BlockingQueuedConnection);
    }

    emit statusChanged(QStringLiteral("Stopped"));
}

bool LiveStreamPlayer::isRunning() const
{
    return m_running.load();
}

void LiveStreamPlayer::requestStop()
{
    stop();
}

void LiveStreamPlayer::demuxLoop(QString url)
{
    int retryCount = 0;
    while (m_running.load())
    {
        if (!openStream(url))
        {
            if (!m_running.load() || m_stopRequested.load())
            {
                break;
            }
            // 连接失败计数并检查上限
            retryCount++;
            const int maxRetries = m_maxReconnectAttempts.load(std::memory_order_acquire);
            if (maxRetries >= 0 && retryCount >= maxRetries)
            {
                emit errorOccurred(QStringLiteral("Failed to connect after %1 attempts.").arg(std::max(0, maxRetries)));
                emit statusChanged(QStringLiteral("Stopped"));
                // 停止运行标志，令各线程自然退出
                m_running.store(false);
                m_stopRequested.store(true);
                break;
            }

            emit statusChanged(QStringLiteral("Retrying connection (%1/%2)").arg(retryCount).arg(std::max(0, maxRetries)));
            const int delay = m_reconnectDelayMs.load(std::memory_order_acquire);
            if (delay > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            continue;
        }

        emit statusChanged(QStringLiteral("Playing"));
        // 成功打开后清零失败计数
        retryCount = 0;

        auto windowStart = std::chrono::steady_clock::now();
        size_t bytesAccumulated = 0;

        while (m_running.load())
        {
            AVPacket packet{};
            int ret = av_read_frame(m_formatCtx, &packet);
            if (ret >= 0)
            {
                bytesAccumulated += static_cast<size_t>(packet.size);
                bool pushed = false;
                if (packet.stream_index == m_videoStreamIndex)
                {
                    pushed = m_videoQueue.push(&packet, m_running);
                }
                else if (packet.stream_index == m_audioStreamIndex)
                {
                    pushed = m_audioQueue.push(&packet, m_running);
                }

                if (!pushed)
                {
                    av_packet_unref(&packet);
                    if (!m_running.load())
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                av_packet_unref(&packet);
            }
            else
            {
                av_packet_unref(&packet);
                if (!m_running.load())
                {
                    break;
                }
                emit statusChanged(QStringLiteral("Connection lost"));
                break;
            }

            auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - windowStart);
            if (elapsed.count() >= 1000)
            {
                double kbps = static_cast<double>(bytesAccumulated) * 8.0 / 1000.0;
                m_bitrateKbps.store(kbps, std::memory_order_relaxed);
                bytesAccumulated = 0;
                windowStart = now;
                updateStats();
            }
        }

        if (!m_running.load() || m_stopRequested.load())
        {
            break;
        }

        // 若仍处于运行状态且不是用户主动停止，则进行重试计数
        if (m_running.load() && !m_stopRequested.load())
        {
            retryCount++;
            const int maxRetries = m_maxReconnectAttempts.load(std::memory_order_acquire);
            if (maxRetries >= 0 && retryCount >= maxRetries)
            {
                emit errorOccurred(QStringLiteral("Connection lost. Reached maximum %1 retries.").arg(std::max(0, maxRetries)));
                emit statusChanged(QStringLiteral("Stopped"));
                m_running.store(false);
                m_stopRequested.store(true);
                // 继续执行清理，之后跳出外层循环
            }
        }

        m_videoQueue.close();
        m_audioQueue.close();
        clearQueues();
        closeStream();

        if (!m_running.load() || m_stopRequested.load())
        {
            break;
        }

        m_videoQueue.open();
        m_audioQueue.open();

        if (m_running.load())
        {
            const int maxRetries = m_maxReconnectAttempts.load(std::memory_order_acquire);
            emit statusChanged(QStringLiteral("Retrying connection (%1/%2)").arg(std::min(retryCount, std::max(0, maxRetries))).arg(std::max(0, maxRetries)));
            const int delay = m_reconnectDelayMs.load(std::memory_order_acquire);
            if (delay > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
}

void LiveStreamPlayer::setMaxReconnectAttempts(int attempts)
{
    if (attempts < 0)
        attempts = 0;
    m_maxReconnectAttempts.store(attempts, std::memory_order_release);
}

void LiveStreamPlayer::setReconnectDelayMs(int delayMs)
{
    if (delayMs < 0)
        delayMs = 0;
    m_reconnectDelayMs.store(delayMs, std::memory_order_release);
}

void LiveStreamPlayer::videoDecodeLoop()
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        emit errorOccurred(QStringLiteral("Failed to allocate video frame."));
        return;
    }

    while (m_running.load())
    {
        AVPacket packet{};
        if (!m_videoQueue.pop(packet, m_running))
        {
            if (!m_running.load())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        QImage frameImage;

        {
            std::lock_guard<std::mutex> lock(m_contextMutex);
            if (!m_videoCodecCtx || !m_swsCtx)
            {
                av_packet_unref(&packet);
                continue;
            }

            int ret = avcodec_send_packet(m_videoCodecCtx, &packet);
            av_packet_unref(&packet);
            if (ret < 0)
            {
                continue;
            }

            while (ret >= 0 && m_running.load())
            {
                ret = avcodec_receive_frame(m_videoCodecCtx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                if (ret < 0)
                {
                    emit errorOccurred(QStringLiteral("Error while decoding video frame."));
                    break;
                }

                QImage image(m_videoCodecCtx->width, m_videoCodecCtx->height, QImage::Format_ARGB32);
                if (image.isNull())
                {
                    continue;
                }

                uint8_t *destData[4] = {image.bits(), nullptr, nullptr, nullptr};
                int destLinesize[4] = {image.bytesPerLine(), 0, 0, 0};

                sws_scale(m_swsCtx,
                          frame->data,
                          frame->linesize,
                          0,
                          m_videoCodecCtx->height,
                          destData,
                          destLinesize);

                frameImage = image.copy();
                av_frame_unref(frame);
                break;
            }
        }

        if (!frameImage.isNull())
        {
            emit frameReady(frameImage);
        }
    }

    av_frame_free(&frame);
}

void LiveStreamPlayer::audioDecodeLoop()
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        emit errorOccurred(QStringLiteral("Failed to allocate audio frame."));
        return;
    }

    while (m_running.load())
    {
        AVPacket packet{};
        if (!m_audioQueue.pop(packet, m_running))
        {
            if (!m_running.load())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        std::vector<QByteArray> pendingSamples;

        {
            std::lock_guard<std::mutex> lock(m_contextMutex);
            const int currentSampleRate = m_targetSampleRate.load(std::memory_order_acquire);
            const int currentChannels = m_targetChannels.load(std::memory_order_acquire);

            if (!m_audioCodecCtx || !m_swrCtx || currentSampleRate <= 0 || currentChannels <= 0)
            {
                av_packet_unref(&packet);
                continue;
            }

            int ret = avcodec_send_packet(m_audioCodecCtx, &packet);
            av_packet_unref(&packet);
            if (ret < 0)
            {
                continue;
            }

            while (ret >= 0 && m_running.load())
            {
                ret = avcodec_receive_frame(m_audioCodecCtx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                if (ret < 0)
                {
                    emit errorOccurred(QStringLiteral("Error while decoding audio frame."));
                    break;
                }

                const int maxSamples = swr_get_out_samples(m_swrCtx, frame->nb_samples);
                const int bufferSize = av_samples_get_buffer_size(nullptr,
                                                                  currentChannels,
                                                                  maxSamples,
                                                                  AV_SAMPLE_FMT_S16,
                                                                  1);
                if (bufferSize <= 0)
                {
                    av_frame_unref(frame);
                    continue;
                }

                QByteArray samples(bufferSize, 0);
                uint8_t *destData[1] = {reinterpret_cast<uint8_t *>(samples.data())};

                int convertedSamples = swr_convert(m_swrCtx,
                                                   destData,
                                                   maxSamples,
                                                   const_cast<const uint8_t **>(frame->extended_data),
                                                   frame->nb_samples);
                if (convertedSamples <= 0)
                {
                    av_frame_unref(frame);
                    continue;
                }

                const int convertedSize = av_samples_get_buffer_size(nullptr,
                                                                     currentChannels,
                                                                     convertedSamples,
                                                                     AV_SAMPLE_FMT_S16,
                                                                     1);
                samples.resize(convertedSize);
                pendingSamples.push_back(samples);
                av_frame_unref(frame);
            }
        }

        for (const QByteArray &samples : pendingSamples)
        {
            emitAudioSamples(samples);
        }
    }

    av_frame_free(&frame);
}

bool LiveStreamPlayer::openStream(const QString &url)
{
    closeStream();

    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext)
    {
        emit errorOccurred(QStringLiteral("Unable to allocate format context."));
        return false;
    }

    formatContext->flags |= AVFMT_FLAG_NOBUFFER;
    formatContext->interrupt_callback.callback = &LiveStreamPlayer::interruptCallback;
    formatContext->interrupt_callback.opaque = this;

    AVDictionary *options = nullptr;
    // Common low-latency flags
    av_dict_set(&options, "buffer_size", "65536", 0);
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "flags", "low_delay", 0);
    // Prefer protocol-agnostic I/O timeout (microseconds)
    av_dict_set(&options, "rw_timeout", QString::number(kDemuxTimeoutUs).toUtf8().constData(), 0);

    const QString scheme = urlSchemeLower(url);
    if (scheme == QLatin1String("rtsp"))
    {
        // Force TCP for RTSP
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        // Keep stimeout for legacy RTSP handling (harmless if ignored)
        av_dict_set(&options, "stimeout", QString::number(kDemuxTimeoutUs).toUtf8().constData(), 0);
    }

    int ret = avformat_open_input(&formatContext, url.toUtf8().constData(), nullptr, &options);
    av_dict_free(&options);
    if (ret < 0)
    {
        emit errorOccurred(QStringLiteral("Failed to open stream: %1").arg(ffmpegErrorString(ret)));
        avformat_free_context(formatContext);
        return false;
    }

    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0)
    {
        emit errorOccurred(QStringLiteral("Failed to retrieve stream info: %1").arg(ffmpegErrorString(ret)));
        avformat_close_input(&formatContext);
        return false;
    }

    int localVideoIndex = -1;
    int localAudioIndex = -1;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
    {
        AVStream *stream = formatContext->streams[i];
        switch (stream->codecpar->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            if (localVideoIndex == -1)
            {
                localVideoIndex = static_cast<int>(i);
            }
            break;
        case AVMEDIA_TYPE_AUDIO:
            if (localAudioIndex == -1)
            {
                localAudioIndex = static_cast<int>(i);
            }
            break;
        default:
            break;
        }
    }

    if (localVideoIndex == -1)
    {
        emit errorOccurred(QStringLiteral("No video stream found."));
        avformat_close_input(&formatContext);
        return false;
    }

    const AVCodec *videoCodec = avcodec_find_decoder(formatContext->streams[localVideoIndex]->codecpar->codec_id);
    if (!videoCodec)
    {
        emit errorOccurred(QStringLiteral("Unsupported video codec."));
        avformat_close_input(&formatContext);
        return false;
    }

    AVCodecContext *videoCodecCtx = avcodec_alloc_context3(videoCodec);
    if (!videoCodecCtx)
    {
        emit errorOccurred(QStringLiteral("Unable to allocate video codec context."));
        avformat_close_input(&formatContext);
        return false;
    }

    ret = avcodec_parameters_to_context(videoCodecCtx, formatContext->streams[localVideoIndex]->codecpar);
    if (ret < 0)
    {
        emit errorOccurred(QStringLiteral("Failed to populate video codec context."));
        avcodec_free_context(&videoCodecCtx);
        avformat_close_input(&formatContext);
        return false;
    }

    videoCodecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    videoCodecCtx->thread_type = FF_THREAD_FRAME;
    videoCodecCtx->thread_count = 1;

    ret = avcodec_open2(videoCodecCtx, videoCodec, nullptr);
    if (ret < 0)
    {
        emit errorOccurred(QStringLiteral("Unable to open video codec."));
        avcodec_free_context(&videoCodecCtx);
        avformat_close_input(&formatContext);
        return false;
    }

    AVCodecContext *audioCodecCtx = nullptr;
    if (localAudioIndex != -1)
    {
        const AVCodec *audioCodec = avcodec_find_decoder(formatContext->streams[localAudioIndex]->codecpar->codec_id);
        if (audioCodec)
        {
            audioCodecCtx = avcodec_alloc_context3(audioCodec);
            if (audioCodecCtx)
            {
                if (avcodec_parameters_to_context(audioCodecCtx, formatContext->streams[localAudioIndex]->codecpar) >= 0)
                {
                    audioCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_S16;
                    ret = avcodec_open2(audioCodecCtx, audioCodec, nullptr);
                    if (ret < 0)
                    {
                        emit errorOccurred(QStringLiteral("Unable to open audio codec."));
                        avcodec_free_context(&audioCodecCtx);
                        audioCodecCtx = nullptr;
                    }
                }
                else
                {
                    emit errorOccurred(QStringLiteral("Failed to populate audio codec context."));
                    avcodec_free_context(&audioCodecCtx);
                    audioCodecCtx = nullptr;
                }
            }
        }
    }

    SwsContext *swsCtx = sws_getContext(videoCodecCtx->width,
                                        videoCodecCtx->height,
                                        videoCodecCtx->pix_fmt,
                                        videoCodecCtx->width,
                                        videoCodecCtx->height,
                                        AV_PIX_FMT_BGRA,
                                        SWS_BILINEAR,
                                        nullptr,
                                        nullptr,
                                        nullptr);
    if (!swsCtx)
    {
        emit errorOccurred(QStringLiteral("Failed to create scaler context."));
        avcodec_free_context(&videoCodecCtx);
        if (audioCodecCtx)
        {
            avcodec_free_context(&audioCodecCtx);
        }
        avformat_close_input(&formatContext);
        return false;
    }

    SwrContext *swrCtx = nullptr;
    if (audioCodecCtx)
    {
        int requestedSampleRate = audioCodecCtx->sample_rate;
        if (requestedSampleRate <= 0)
        {
            requestedSampleRate = formatContext->streams[localAudioIndex]->codecpar->sample_rate;
        }
        if (requestedSampleRate <= 0)
        {
            requestedSampleRate = 48000;
        }

        int requestedChannels = audioCodecCtx->ch_layout.nb_channels;
        if (requestedChannels <= 0)
        {
            requestedChannels = formatContext->streams[localAudioIndex]->codecpar->ch_layout.nb_channels;
        }
        if (requestedChannels <= 0)
        {
            requestedChannels = 2;
        }

        setupAudioOutput(requestedSampleRate, requestedChannels);

        const int actualSampleRate = m_targetSampleRate.load(std::memory_order_acquire);
        const int actualChannels = m_targetChannels.load(std::memory_order_acquire);

        if (actualSampleRate > 0 && actualChannels > 0)
        {
            AVChannelLayout inputLayout{};
            AVChannelLayout outputLayout{};
            bool inputLayoutInitialized = false;
            bool outputLayoutInitialized = false;

            int layoutResult = 0;
            if (audioCodecCtx->ch_layout.nb_channels > 0)
            {
                layoutResult = av_channel_layout_copy(&inputLayout, &audioCodecCtx->ch_layout);
                if (layoutResult >= 0)
                    inputLayoutInitialized = true;
            }
            else if (formatContext->streams[localAudioIndex]->codecpar->ch_layout.nb_channels > 0)
            {
                layoutResult = av_channel_layout_copy(&inputLayout, &formatContext->streams[localAudioIndex]->codecpar->ch_layout);
                if (layoutResult >= 0)
                    inputLayoutInitialized = true;
            }
            else
            {
                layoutResult = ff_channel_layout_default_compat(&inputLayout, requestedChannels);
                if (layoutResult >= 0)
                    inputLayoutInitialized = true;
            }

            if (layoutResult < 0)
            {
                if (inputLayoutInitialized)
                    av_channel_layout_uninit(&inputLayout);
                emit errorOccurred(QStringLiteral("Failed to resolve input audio channel layout."));
                avcodec_free_context(&videoCodecCtx);
                avcodec_free_context(&audioCodecCtx);
                avformat_close_input(&formatContext);
                teardownAudioOutput();
                return false;
            }

            if (ff_channel_layout_default_compat(&outputLayout, actualChannels) < 0)
            {
                if (inputLayoutInitialized)
                    av_channel_layout_uninit(&inputLayout);
                emit errorOccurred(QStringLiteral("Failed to prepare output audio channel layout."));
                avcodec_free_context(&videoCodecCtx);
                avcodec_free_context(&audioCodecCtx);
                avformat_close_input(&formatContext);
                teardownAudioOutput();
                return false;
            }
            else
            {
                outputLayoutInitialized = true;
            }

            if (swr_alloc_set_opts2(&swrCtx,
                                    &outputLayout,
                                    AV_SAMPLE_FMT_S16,
                                    actualSampleRate,
                                    &inputLayout,
                                    audioCodecCtx->sample_fmt,
                                    audioCodecCtx->sample_rate,
                                    0,
                                    nullptr) < 0 ||
                swr_init(swrCtx) < 0)
            {
                if (swrCtx)
                {
                    swr_free(&swrCtx);
                }
                av_channel_layout_uninit(&inputLayout);
                av_channel_layout_uninit(&outputLayout);
                emit errorOccurred(QStringLiteral("Failed to initialise audio resampler."));
                avcodec_free_context(&videoCodecCtx);
                avcodec_free_context(&audioCodecCtx);
                avformat_close_input(&formatContext);
                teardownAudioOutput();
                return false;
            }

            if (inputLayoutInitialized)
                av_channel_layout_uninit(&inputLayout);
            if (outputLayoutInitialized)
                av_channel_layout_uninit(&outputLayout);
        }
        else
        {
            emit errorOccurred(QStringLiteral("Audio output initialisation failed."));
            avcodec_free_context(&audioCodecCtx);
            audioCodecCtx = nullptr;
            teardownAudioOutput();
        }
    }
    else
    {
        teardownAudioOutput();
    }

    {
        std::lock_guard<std::mutex> guard(m_contextMutex);
        m_formatCtx = formatContext;
        m_videoCodecCtx = videoCodecCtx;
        m_audioCodecCtx = audioCodecCtx;
        m_swsCtx = swsCtx;
        m_swrCtx = swrCtx;
        m_videoStreamIndex = localVideoIndex;
        m_audioStreamIndex = localAudioIndex;
        m_videoFrameDurationMs = 0.0;
        m_audioFrameDurationMs = 0.0;

        if (m_videoCodecCtx)
        {
            double fps = av_q2d(m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate);
            if (fps < 1.0)
            {
                fps = av_q2d(m_formatCtx->streams[m_videoStreamIndex]->r_frame_rate);
            }
            if (fps < 1.0 && m_formatCtx->streams[m_videoStreamIndex]->time_base.den != 0)
            {
                fps = 1.0 / av_q2d(m_formatCtx->streams[m_videoStreamIndex]->time_base);
            }
            if (fps < 1.0)
            {
                fps = 30.0;
            }
            m_videoFrameDurationMs = 1000.0 / fps;
        }

        if (m_audioCodecCtx)
        {
            if (m_audioCodecCtx->frame_size > 0 && m_audioCodecCtx->sample_rate > 0)
            {
                m_audioFrameDurationMs = 1000.0 * static_cast<double>(m_audioCodecCtx->frame_size) /
                                         static_cast<double>(m_audioCodecCtx->sample_rate);
            }
            else if (m_audioCodecCtx->sample_rate > 0)
            {
                m_audioFrameDurationMs = 1000.0 * 1024.0 / static_cast<double>(m_audioCodecCtx->sample_rate);
            }
        }
    }

    return true;
}

void LiveStreamPlayer::closeStream()
{
    std::lock_guard<std::mutex> guard(m_contextMutex);

    if (m_videoCodecCtx)
    {
        avcodec_free_context(&m_videoCodecCtx);
        m_videoCodecCtx = nullptr;
    }

    if (m_audioCodecCtx)
    {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }

    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    if (m_formatCtx)
    {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }

    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_videoFrameDurationMs = 0.0;
    m_audioFrameDurationMs = 0.0;
}

void LiveStreamPlayer::clearQueues()
{
    m_videoQueue.clear();
    m_audioQueue.clear();
}

void LiveStreamPlayer::resetState()
{
    clearQueues();
    closeStream();
}

void LiveStreamPlayer::setupAudioOutput(int sampleRate, int channels)
{
    if (sampleRate <= 0 || channels <= 0)
    {
        return;
    }

    const bool sameThread = thread() == QThread::currentThread();
    const Qt::ConnectionType connectionType = sameThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod(this, [this, sampleRate, channels]()
                              {
        if (m_audioOutput) {
            m_audioOutput->stop();
            delete m_audioOutput;
            m_audioOutput = nullptr;
            m_audioDevice = nullptr;
        }

        QAudioFormat outputFormat;
        outputFormat.setCodec("audio/pcm");
        outputFormat.setChannelCount(channels);
        outputFormat.setSampleRate(sampleRate);
        outputFormat.setSampleSize(16);
        outputFormat.setSampleType(QAudioFormat::SignedInt);
        outputFormat.setByteOrder(QAudioFormat::LittleEndian);

        QAudioDeviceInfo device = QAudioDeviceInfo::defaultOutputDevice();
        if (!device.isFormatSupported(outputFormat)) {
            outputFormat = device.nearestFormat(outputFormat);
        }

    m_targetSampleRate.store(outputFormat.sampleRate(), std::memory_order_release);
    m_targetChannels.store(outputFormat.channelCount(), std::memory_order_release);

    m_audioOutput = new QAudioOutput(device, outputFormat, this);
        m_audioOutput->setBufferSize(outputFormat.sampleRate() * outputFormat.channelCount() * 2 / 5);
        m_audioDevice = m_audioOutput->start(); }, connectionType);
}

void LiveStreamPlayer::teardownAudioOutput()
{
    if (thread() != QThread::currentThread())
    {
        QMetaObject::invokeMethod(this, [this]()
                                  { teardownAudioOutput(); }, Qt::BlockingQueuedConnection);
        return;
    }

    if (m_audioOutput)
    {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
        m_audioDevice = nullptr;
    }

    m_targetSampleRate.store(0, std::memory_order_release);
    m_targetChannels.store(0, std::memory_order_release);
}

void LiveStreamPlayer::emitAudioSamples(QByteArray samples)
{
    QMetaObject::invokeMethod(this, [this, samples]()
                              {
        if (!m_audioOutput || !m_audioDevice) {
            return;
        }

        const int totalSize = samples.size();
        int offset = 0;

        while (offset < totalSize) {
            if (m_audioOutput->bytesFree() <= 0) {
                break;
            }

            const qint64 written = m_audioDevice->write(samples.constData() + offset, totalSize - offset);
            if (written <= 0) {
                break;
            }

            offset += static_cast<int>(written);
        }

        if (offset < totalSize) {
            const QByteArray remainingSamples = samples.mid(offset);
            QMetaObject::invokeMethod(this, [this, remainingSamples]() { emitAudioSamples(remainingSamples); }, Qt::QueuedConnection);
        } }, Qt::QueuedConnection);
}

void LiveStreamPlayer::updateStats()
{
    PlayerStats stats;
    stats.videoQueueSize = static_cast<int>(m_videoQueue.size());
    stats.audioQueueSize = static_cast<int>(m_audioQueue.size());
    stats.incomingBitrateKbps = m_bitrateKbps.load(std::memory_order_relaxed);

    double jitterVideo = 0.0;
    double jitterAudio = 0.0;

    if (m_videoFrameDurationMs > 0.0)
    {
        jitterVideo = m_videoFrameDurationMs * stats.videoQueueSize;
    }

    if (m_audioFrameDurationMs > 0.0)
    {
        jitterAudio = m_audioFrameDurationMs * stats.audioQueueSize;
    }

    stats.jitterBufferMs = std::max(jitterVideo, jitterAudio);
    emit statsUpdated(stats);
}

int LiveStreamPlayer::interruptCallback(void *opaque)
{
    if (!opaque)
    {
        return 0;
    }

    auto *player = static_cast<LiveStreamPlayer *>(opaque);
    if (!player)
    {
        return 0;
    }

    if (!player->m_running.load(std::memory_order_acquire) || player->m_stopRequested.load(std::memory_order_acquire))
    {
        return 1;
    }

    return 0;
}

QString LiveStreamPlayer::ffmpegErrorString(int errorCode)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errorCode, buffer, sizeof(buffer));
    return QString::fromUtf8(buffer);
}

QString LiveStreamPlayer::sanitizeInputUrl(const QString &url)
{
    QUrl u = QUrl::fromUserInput(url);
    if (!u.isValid())
    {
        return url;
    }

    const QString scheme = u.scheme().toLower();
    // Player is client-only; remove server-listen params for RTMP/TCP URLs if present
    if (scheme == QLatin1String("rtmp") || scheme == QLatin1String("tcp"))
    {
        QUrlQuery q(u);
        bool changed = false;
        if (q.hasQueryItem("listen"))
        {
            q.removeQueryItem("listen");
            changed = true;
        }
        if (q.hasQueryItem("listen_timeout"))
        {
            q.removeQueryItem("listen_timeout");
            changed = true;
        }
        if (changed)
        {
            u.setQuery(q);
            return u.toString();
        }
    }
    return url;
}

QString LiveStreamPlayer::urlSchemeLower(const QString &url)
{
    QUrl u = QUrl::fromUserInput(url);
    if (!u.isValid())
    {
        return QString();
    }
    return u.scheme().toLower();
}
