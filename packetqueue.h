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

// Thread-safe bounded queue that doubles as the jitter buffer between demux and decode stages.
class PacketQueue
{
public:
    explicit PacketQueue(size_t maxPackets = 120);
    ~PacketQueue();

    void setMaxSize(size_t maxPackets);
    bool push(const AVPacket *packet, std::atomic_bool &running);
    bool pop(AVPacket &outPacket, std::atomic_bool &running);
    void clear();
    void close();
    void open();
    bool isOpen() const;
    size_t size() const;

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cvNotEmpty;
    std::condition_variable m_cvNotFull;
    std::deque<AVPacket> m_queue;
    size_t m_maxSize;
    bool m_closed;
};

#endif // PACKETQUEUE_H
