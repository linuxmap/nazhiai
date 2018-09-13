
#ifndef _FAILEDCLEANTASK_HEADER_H_
#define _FAILEDCLEANTASK_HEADER_H_

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>

class Maria;
class FailedCleanTask
{
public:
    FailedCleanTask();
    ~FailedCleanTask();

private:
    void Clean();
    bool QueryFaceTimepoints(int hostid, const std::string& source, long long beg, long long end, std::vector<long long>& timepoints);

    void CleanShortVideos(Maria& maria, int hostid, const std::string& source, long long beg, long long end);

private:
    bool _cleaning;
    std::thread _task;
    std::mutex _locker;
    std::condition_variable _condition;

private:
    FailedCleanTask(const FailedCleanTask&);
    FailedCleanTask& operator=(const FailedCleanTask&);
};

#endif

