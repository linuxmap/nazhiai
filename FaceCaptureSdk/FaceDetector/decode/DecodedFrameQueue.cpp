#include "DecodedFrameQueue.h"
#include "TimeStamp.h"
#include "XMatPool.h"

#include "AutoLock.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

DecodedFrameQueue::DecodedFrameQueue(int gpuIndex, int bufferSize)
    : _gpuIndex(gpuIndex), _bufferSize(bufferSize), _warningSize(0), _lastDiscardClock(clock())
    , _decodedFrameQueuesLocker(), _decodedFrameQueuesCondition(), _decodedFrameQueues()
{

}

DecodedFrameQueue::~DecodedFrameQueue()
{
    AUTOLOCK(_decodedFrameQueuesLocker);
    while (_decodedFrameQueues.size() > 0)
    {
        Reserve(_decodedFrameQueues.front());
        _decodedFrameQueues.pop();
    }
}

void DecodedFrameQueue::Push(DecodedFrame& decodedFrame)
{
    AUTOLOCK(_decodedFrameQueuesLocker);
    _decodedFrameQueues.push(decodedFrame);
    if (_decodedFrameQueues.size() > _bufferSize)
    {
        DecodedFrame& alias = _decodedFrameQueues.front();
        Reserve(alias);
        _decodedFrameQueues.pop();

        if (++_warningSize >= 10)
        {
            LOG(WARNING) << _warningSize << " frames of (" << decodedFrame.sourceId << ") were discarded, because of buffer overflow: " << _bufferSize;
            _warningSize = 0;
        }
        else if (_warningSize == 1)
        {
            _lastDiscardClock = clock();
        }
    }

    if (_warningSize > 0 && _lastDiscardClock + 5000 < clock())
    {
        LOG(WARNING) << _warningSize << " frames of (" << decodedFrame.sourceId << ") were discarded(5 seconds ahead), because of buffer overflow: " << _bufferSize;
        _warningSize = 0;
    }

    WAKEUP_ONE(_decodedFrameQueuesCondition);
}

bool DecodedFrameQueue::HasMore()
{
    AUTOLOCK(_decodedFrameQueuesLocker);
    return _decodedFrameQueues.size() > 0;
}

bool DecodedFrameQueue::Pop(DecodedFrame& decodedFrame, bool synchronize)
{
    if (synchronize)
    {
        WAIT_MILLISEC_TILL_COND(_decodedFrameQueuesCondition, _decodedFrameQueuesLocker, 1000, [this]{ return _decodedFrameQueues.size() > 0; });
        if (_decodedFrameQueues.size() > 0)
        {
            decodedFrame = _decodedFrameQueues.front();
            _decodedFrameQueues.pop();
            return true;
        }
    }
    else
    {
        AUTOLOCK(_decodedFrameQueuesLocker);
        if (_decodedFrameQueues.size() > 0)
        {
            decodedFrame = _decodedFrameQueues.front();
            _decodedFrameQueues.pop();
            return true;
        }
    }
    return false;
}

void DecodedFrameQueue::Reserve(DecodedFrame& decodedFrame)
{
    if (decodedFrame.buffered)
    {
        if (!decodedFrame.mat.empty())
        {
            XMatPool<cv::Mat>::Free(decodedFrame.mat, _gpuIndex);
        }
        else if (!decodedFrame.gpumat.empty())
        {
            XMatPool<cv::cuda::GpuMat>::Free(decodedFrame.gpumat, _gpuIndex);
        }
    }
}

