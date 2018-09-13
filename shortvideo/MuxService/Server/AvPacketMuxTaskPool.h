
#ifndef _AVPACKETMUXTASKPOOL_HEADER_H_
#define _AVPACKETMUXTASKPOOL_HEADER_H_

#include "AvPacketMuxTask.h"

class AvPacketMuxTaskPool
{
public:
    AvPacketMuxTaskPool();
    ~AvPacketMuxTaskPool();

    AvPacketMuxTask* Alloc(int hostId, const std::string& url, AVFormatContext** from, int width, int height, int frameRate);
    void Free(AvPacketMuxTask* task);
    
private:
    std::mutex _locker;
    std::queue<AvPacketMuxTask*> _tasks;
};

#endif

