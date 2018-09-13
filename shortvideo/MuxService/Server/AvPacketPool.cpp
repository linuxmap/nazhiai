#include "AvPacketPool.h"

std::mutex AvPacketPool::_locker;
std::queue<AVPacket*> AvPacketPool::_pool;

AVPacket* AvPacketPool::Alloc()
{
    {
        std::lock_guard<std::mutex> lg(_locker);
        if (_pool.size() > 0)
        {
            AVPacket* packet = _pool.front();
            _pool.pop();
            return packet;
        }
    }
    return av_packet_alloc();
}

void AvPacketPool::Free(AVPacket* packet)
{
    av_packet_unref(packet);
    std::lock_guard<std::mutex> lg(_locker);
    _pool.push(packet);
}

bool AvPacketPool::Initialize()
{
    return true;
}

void AvPacketPool::Destroy()
{
    std::lock_guard<std::mutex> lg(_locker);
    while (_pool.size() > 0)
    {
        AVPacket* packet = _pool.front();
        av_packet_free(&packet);
        _pool.pop();
    }
}

