#include "AvPacketMuxTaskPool.h"

AvPacketMuxTaskPool::AvPacketMuxTaskPool()
    : _locker(), _tasks()
{
}

AvPacketMuxTaskPool::~AvPacketMuxTaskPool()
{
    std::lock_guard<std::mutex> lg(_locker);
    while (!_tasks.empty())
    {
        AvPacketMuxTask* task = _tasks.front();
        _tasks.pop();

        if (task)
        {
            task->WaitToStop();
            delete task;
            task = nullptr;
        }
    }
}

AvPacketMuxTask* AvPacketMuxTaskPool::Alloc(int hostId, const std::string& url, AVFormatContext** from, int width, int height, int frameRate)
{
    {
        std::lock_guard<std::mutex> lg(_locker);
        if (!_tasks.empty())
        {
            AvPacketMuxTask* task = _tasks.front();
            _tasks.pop();
            return task;
        }
    }
    return new AvPacketMuxTask(hostId, url, from, width, height, frameRate, *this);
}

void AvPacketMuxTaskPool::Free(AvPacketMuxTask* task)
{
    if (task)
    {
        std::lock_guard<std::mutex> lg(_locker);
        _tasks.push(task);
    }
}
