#ifndef _FPS_HEADER_H_
#define _FPS_HEADER_H_

#include <time.h>
#include <mutex>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

class FPStat
{
public:
    FPStat(clock_t duration, bool multithread = false) : _count(0), _begin(clock()), _duration(duration), _multithread(multithread)
    {
        _begin += _duration;
        _seconds = _duration / 1000.0f;
    }

    void Stat(size_t add, const std::string& msg = "")
    {
        if (_multithread)
        {
            _locker.lock();
        }

        _count += add;
        if (clock() >= _begin)
        {
            LOG(INFO) << msg << " FPS: " << _count / _seconds;
            _begin = clock() + _duration;
            _count = 0;
        }

        if (_multithread)
        {
            _locker.unlock();
        }
    }

private:
    size_t _count;
    double _seconds;
    clock_t _begin;
    clock_t _duration;

    bool _multithread;
    std::mutex _locker;
};

#if defined(LOG_FPS)

#define DECLARE_FPS(ms) FPStat fpstat(ms)
#define DECLARE_FPS_STATIC(ms) static FPStat fpstat(ms, true)
#define STATISTIC_FPS(size, msg) fpstat.Stat(size, msg)

#else

#define DECLARE_FPS(ms) 
#define DECLARE_FPS_STATIC(ms)
#define STATISTIC_FPS(size, msg) 

#endif

#endif


