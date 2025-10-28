#include "packetqueue.h"

PacketQueue::PacketQueue(size_t maxPackets)
    : m_maxSize(maxPackets), m_closed(false)
{
}

PacketQueue::~PacketQueue()
{
    close();
    clear();
}

void PacketQueue::setMaxSize(size_t maxPackets)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxSize = maxPackets;
    m_cvNotFull.notify_all();
}

bool PacketQueue::push(const AVPacket *packet, std::atomic_bool &running)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvNotFull.wait(lock, [this, &running]()
                     { return m_closed || m_queue.size() < m_maxSize || !running.load(); });

    if (m_closed || !running.load())
    {
        return false;
    }

    AVPacket copy{};
    if (av_packet_ref(&copy, packet) < 0)
    {
        return false;
    }

    m_queue.push_back(copy);
    m_cvNotEmpty.notify_one();
    return true;
}

bool PacketQueue::pop(AVPacket &outPacket, std::atomic_bool &running)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvNotEmpty.wait(lock, [this, &running]()
                      { return m_closed || !m_queue.empty() || !running.load(); });

    if (m_queue.empty())
    {
        return false;
    }

    AVPacket packet = m_queue.front();
    m_queue.pop_front();
    av_packet_move_ref(&outPacket, &packet);
    av_packet_unref(&packet);
    m_cvNotFull.notify_one();
    return true;
}

void PacketQueue::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (AVPacket &packet : m_queue)
    {
        av_packet_unref(&packet);
    }
    m_queue.clear();
    m_cvNotFull.notify_all();
}

void PacketQueue::close()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
    }
    m_cvNotEmpty.notify_all();
    m_cvNotFull.notify_all();
}

void PacketQueue::open()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = false;
    }
    m_cvNotFull.notify_all();
}

bool PacketQueue::isOpen() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_closed;
}

size_t PacketQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}
