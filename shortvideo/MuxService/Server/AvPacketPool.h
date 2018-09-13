
#ifndef _AVPACKETPOOL_HEADER_H_
#define _AVPACKETPOOL_HEADER_H_

#pragma warning(disable:4819)

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif

#pragma warning(default:4819)

#include <mutex>
#include <queue>

class AvPacketPool
{
public:
    static AVPacket* Alloc();
    static void Free(AVPacket* packet);

    static bool Initialize();
    static void Destroy();

private:
    static std::mutex _locker;
    static std::queue<AVPacket*> _pool;

private:
    AvPacketPool();
    AvPacketPool(const AvPacketPool&);
    AvPacketPool& operator=(const AvPacketPool&);
};

#endif

