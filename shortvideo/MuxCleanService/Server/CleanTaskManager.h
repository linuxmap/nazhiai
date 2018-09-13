
#ifndef _CLEANTASKMANAGER_HEADER_H_
#define _CLEANTASKMANAGER_HEADER_H_

#include "RealtimeCleanTask.h"

#include <mutex>
#include <map>

class CleanTaskManager
{
public:
    CleanTaskManager(int hostid);
    ~CleanTaskManager();

    void Startup();
    void Destroy();

private:
    void UpdateActiveTask();
    void UpdateDeactiveTask();

private:
    int _hostid;
    std::map<std::string, RealtimeCleanTask*> _tasks;
    std::mutex _locker;

    bool _running;
    std::thread _active;
    std::thread _deactive;

private:
    CleanTaskManager();
    CleanTaskManager(const CleanTaskManager&);
    CleanTaskManager& operator=(const CleanTaskManager&);
};

#endif

