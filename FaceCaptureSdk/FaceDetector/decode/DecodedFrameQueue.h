
#ifndef _DECODEDFRAMEQUEUE_HEADER_H_
#define _DECODEDFRAMEQUEUE_HEADER_H_

#include "DecodedFrame.h"

#include <memory>
#include <mutex>
#include <condition_variable>

class DecodedFrameQueue
{
public:
    DecodedFrameQueue(int gpuIndex, int buferSize);
    ~DecodedFrameQueue();

    void Push(DecodedFrame& decodedFrame);
    bool HasMore();
    bool Pop(DecodedFrame& decodedFrame, bool synchronize = false);

private:
    void Reserve(DecodedFrame& decodedFrame);

private:
    int _gpuIndex;
    int _bufferSize;

    std::mutex _decodedFrameQueuesLocker;
    std::condition_variable _decodedFrameQueuesCondition;
    std::queue<DecodedFrame> _decodedFrameQueues;

private:
    DecodedFrameQueue();
    DecodedFrameQueue(const DecodedFrameQueue&);
    DecodedFrameQueue& operator=(const DecodedFrameQueue&);
};
typedef std::shared_ptr<DecodedFrameQueue> DecodedFrameQueuePtr;

#endif

