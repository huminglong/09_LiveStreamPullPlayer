/**
 * @file packetqueue.h
 * @brief 定义线程安全的 FFmpeg AVPacket 队列。
 * @mainfunctions
 *   - push
 *   - pop
 *   - clear
 *   - open
 *   - close
 *   - isOpen
 *   - size
 * @mainclasses
 *   - PacketQueue
 */

#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

extern "C"
{
#include <libavcodec/avcodec.h>
}

/**
 * @brief PacketQueue 提供线程安全的有界 AVPacket 缓存。
 */
class PacketQueue {
public:
    enum class OverflowPolicy {
        Block,
        DropOldest
    };

    /**
     * @brief 构造函数，可指定最大包数量。
     * @param maxPackets 队列容量。
     * @param policy 队列写满时的溢出策略。
     */
    explicit PacketQueue(size_t maxPackets = 120, OverflowPolicy policy = OverflowPolicy::Block);

    /**
     * @brief 析构函数，负责清理资源。
     */
    ~PacketQueue();

    /**
     * @brief 调整队列容量并唤醒等待者。
     * @param maxPackets 新容量。
     */
    void setMaxSize(size_t maxPackets);

    /**
     * @brief 将 AVPacket 推入队列，必要时阻塞等待。
     * @param packet 待复制的包。
     * @param running 播放器运行标志。
     * @return true 表示成功。
     */
    bool push(const AVPacket* packet, std::atomic_bool& running);

    /**
     * @brief 从队列取出一个包。
     * @param outPacket 输出参数。
     * @param running 播放器运行标志。
     * @return true 表示成功取包。
     */
    bool pop(AVPacket& outPacket, std::atomic_bool& running);

    /**
     * @brief 清空队列并释放内部 AVPacket。
     */
    void clear();

    /**
     * @brief 标记队列关闭并唤醒等待的线程。
     */
    void close();

    /**
     * @brief 重新开启队列以允许读写。
     */
    void open();

    /**
     * @brief 查询队列是否处于开启状态。
     * @return true 表示可用。
     */
    bool isOpen() const;

    /**
     * @brief 获取当前缓存的包数量。
     * @return 队列长度。
     */
    size_t size() const;

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cvNotEmpty;
    std::condition_variable m_cvNotFull;
    std::deque<AVPacket> m_queue;
    size_t m_maxSize;
    bool m_closed;
    OverflowPolicy m_policy;
};

#endif // PACKETQUEUE_H
