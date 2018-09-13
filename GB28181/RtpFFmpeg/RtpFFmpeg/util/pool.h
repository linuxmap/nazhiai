
#ifndef _POOL_HEADER_H_
#define _POOL_HEADER_H_

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename El>
class Pool
{
public:
    static bool Alloc(El& el, bool asyn = false, long long timeout = 0x05265C00)
    {
        if (asyn)
        {
            std::lock_guard<std::mutex> lg(_locker);
            if (_queue.size() > 0)
            {
                el = _queue.front();
                _queue.pop();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            std::unique_lock<std::mutex> ul(_locker);
            _condition.wait_for(ul, std::chrono::milliseconds(timeout), [this]() { return _queue.size() > 0; });

            if (_queue.size() > 0)
            {
                el = _queue.front();
                _queue.pop();
                return true;
            } 
            else
            {
                return false;
            }
        }
    }

    static void Free(El& el)
    {
        std::lock_guard<std::mutex> lg(_locker);
        _queue.push(el);
    }

private:
    static std::queue<El> _queue;
    static std::mutex _locker;
    static std::condition_variable _condition;

private:
    Pool();
    Pool(const Pool&);
    Pool& operator=(const Pool&);
};

template <typename El>
std::queue<El> Pool<El>::_queue;

template <typename El>
std::mutex Pool<El>::_locker;

template <typename El>
std::condition_variable Pool<El>::_condition;

#endif

