
#ifndef _TIMESTAMP_HEADER_H_
#define _TIMESTAMP_HEADER_H_

#include <chrono>

enum time_stamp_precision { SECONDS, MILLISECONDS, MICROSECONDS };

template<time_stamp_precision Precision = MILLISECONDS>
class TimeStamp
{
public:
    static long long Now()
    {
        typedef std::chrono::milliseconds time_precision;
        typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
        now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
        return tp.time_since_epoch().count();
    }
};

template<>
class TimeStamp<SECONDS>
{
public:
    static long long Now()
    {
        typedef std::chrono::seconds time_precision;
        typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
        now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
        return tp.time_since_epoch().count();
    }
};

template<>
class TimeStamp<MICROSECONDS>
{
public:
    static long long Now()
    {
        typedef std::chrono::microseconds time_precision;
        typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
        now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
        return tp.time_since_epoch().count();
    }
};

#endif

