
#ifndef _CUSTOMCLEANTASK_HEADER_H_
#define _CUSTOMCLEANTASK_HEADER_H_

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>

class TriggerCleanTask
{
public:
    TriggerCleanTask();
    ~TriggerCleanTask();

    bool TaskConflict(int hostid, const std::string& source, const std::string& begin, const std::string& end);

    void Start(int hostid, const std::string& source, const std::string& begintime, const std::string& endtime);

private:
    void Clean();
    bool QueryFaceTimepoints(long long beg, long long end, std::vector<long long>& timepoints);

    void CleanShortVideos(long long beg, long long end);

private:
    int _hostid;
    std::string _source;

    std::string _begintime;
    std::string _endtime;

    bool _cleanning;
    std::thread _task;
    std::mutex _locker;
    std::condition_variable _condition;

private:
    TriggerCleanTask(const TriggerCleanTask&);
    TriggerCleanTask& operator=(const TriggerCleanTask&);
};


#endif

