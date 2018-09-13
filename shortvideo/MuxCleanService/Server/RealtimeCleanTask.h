
#ifndef _REALTIMECLEANTASK_HEADER_H_
#define _REALTIMECLEANTASK_HEADER_H_

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>

class RealtimeCleanTask
{
public:
    RealtimeCleanTask(int hostid, const std::string& source);
    ~RealtimeCleanTask();

private:
    void Clean();
    bool QueryFaceTimepoints(long long beg, long long end, std::vector<long long>& timepoints);

    void CleanShortVideos(long long beg, long long end);

private:
    int _hostid;
    std::string _source;
    long long _last_clean_timepoint;

    bool _cleaning;
    std::thread _task;
    std::mutex _locker;
    std::condition_variable _condition;

private:
    RealtimeCleanTask();
    RealtimeCleanTask(const RealtimeCleanTask&);
    RealtimeCleanTask& operator=(const RealtimeCleanTask&);
};

#endif

