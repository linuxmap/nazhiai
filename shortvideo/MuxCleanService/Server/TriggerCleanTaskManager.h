
#ifndef _TRIGERCLEANTASKMANAGER_HEADER_H_
#define _TRIGERCLEANTASKMANAGER_HEADER_H_

#include <mutex>
#include <map>
#include <queue>

class TriggerCleanTask;
class TriggerCleanTaskManager
{
public:
    static bool AddCustomCleanTask(int hostid, const std::string& source, const std::string& begintime, const std::string& endtime, std::string& result);
    static void Destroy();

private:
    static void Free(TriggerCleanTask* task);
    friend class TriggerCleanTask;

private:
    static std::mutex _locker;
    static std::queue<TriggerCleanTask*> _freetasks;
    static std::map<TriggerCleanTask*, TriggerCleanTask*> _customcleantasks;

private:
    TriggerCleanTaskManager();
    TriggerCleanTaskManager(const TriggerCleanTaskManager&);
    TriggerCleanTaskManager& operator=(const TriggerCleanTaskManager&);
};

#endif

