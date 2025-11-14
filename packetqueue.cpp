/**
 * @file packetqueue.cpp
 * @brief 实现线程安全的 AVPacket 队列操作。
 * @mainfunctions
 *   - PacketQueue::push
 *   - PacketQueue::pop
 *   - PacketQueue::clear
 *   - PacketQueue::open
 *   - PacketQueue::close
 *   - PacketQueue::size
 * @mainclasses
 *   - PacketQueue
 */

#include "packetqueue.h"

 /**
  * @brief 构造函数，初始化容量与关闭标志。
  * @param maxPackets 队列容量。
  */
PacketQueue::PacketQueue(size_t maxPackets, OverflowPolicy policy)
    : m_maxSize(maxPackets), m_closed(false), m_policy(policy) {
}

/**
 * @brief 析构函数，确保关闭并清空队列。
 */
PacketQueue::~PacketQueue() {
    close();
    clear();
}

/**
 * @brief 更新队列容量并唤醒等待的生产者。
 * @param maxPackets 新容量。
 */
void PacketQueue::setMaxSize(size_t maxPackets) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxSize = maxPackets;
    if (m_policy == OverflowPolicy::DropOldest) {
        while (m_queue.size() > m_maxSize) {
            AVPacket dropped = m_queue.front();
            m_queue.pop_front();
            av_packet_unref(&dropped);
        }
    }
    m_cvNotFull.notify_all();
}

/**
 * @brief 将包复制后推入队列，必要时阻塞等待。
 * @param packet 待推入的包。
 * @param running 播放器运行标志。
 * @return true 表示成功。
 */
bool PacketQueue::push(const AVPacket* packet, std::atomic_bool& running) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_policy == OverflowPolicy::Block) {
        m_cvNotFull.wait(lock, [this, &running]() { return m_closed || m_queue.size() < m_maxSize || !running.load(); });
        if (m_closed || !running.load()) {
            return false;
        }
    }
    else {
        while (!m_closed && running.load() && m_queue.size() >= m_maxSize) {
            // 丢弃最旧的包以控制延迟，避免生产者线程停顿
            AVPacket dropped = m_queue.front();
            m_queue.pop_front();
            av_packet_unref(&dropped);
        }
        if (m_closed || !running.load()) {
            return false;
        }
    }

    AVPacket copy{};
    if (av_packet_ref(&copy, packet) < 0) {
        return false;
    }

    m_queue.push_back(copy);
    m_cvNotEmpty.notify_one();
    return true;
}

/**
 * @brief 从队列取出一个包，必要时阻塞等待。
 * @param outPacket 输出引用。
 * @param running 播放器运行标志。
 * @return true 表示成功取出。
 */
bool PacketQueue::pop(AVPacket& outPacket, std::atomic_bool& running) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvNotEmpty.wait(lock, [this, &running]() { return m_closed || !m_queue.empty() || !running.load(); });

    if (m_queue.empty()) {
        return false;
    }

    AVPacket packet = m_queue.front();
    m_queue.pop_front();
    av_packet_move_ref(&outPacket, &packet);
    av_packet_unref(&packet);
    m_cvNotFull.notify_one();
    return true;
}

/**
 * @brief 清空队列并释放 AVPacket 引用。
 */
void PacketQueue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (AVPacket& packet : m_queue) {
        av_packet_unref(&packet);
    }
    m_queue.clear();
    m_cvNotFull.notify_all();
}

/**
 * @brief 将队列标记为关闭并通知所有等待者。
 */
void PacketQueue::close() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
    }
    m_cvNotEmpty.notify_all();
    m_cvNotFull.notify_all();
}

/**
 * @brief 重新打开队列以允许生产消费。
 */
void PacketQueue::open() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = false;
    }
    m_cvNotFull.notify_all();
}

/**
 * @brief 返回队列是否可用。
 * @return true 表示开启状态。
 */
bool PacketQueue::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_closed;
}

/**
 * @brief 返回当前缓存的包数量。
 * @return 队列长度。
 */
size_t PacketQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}
