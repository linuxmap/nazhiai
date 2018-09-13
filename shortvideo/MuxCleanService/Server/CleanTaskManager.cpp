#include "CleanTaskManager.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "Maria.h"
#include "Configuration.h"

#include <queue>

CleanTaskManager::CleanTaskManager(int hostid)
    : _hostid(hostid)
    , _tasks(), _locker()
    , _running(false), _active(), _deactive()
{
}

CleanTaskManager::~CleanTaskManager()
{
}

void CleanTaskManager::UpdateActiveTask()
{
    while (_running)
    {
        std::vector<std::string> sources;
        Maria maria(Configuration::db.url);
        if (maria.QuerySourcesByStatus(_hostid, 1, sources))
        {
            std::lock_guard<std::mutex> lg(_locker);
            for each(std::string source in sources)
            {
                if (_tasks.find(source) == _tasks.end())
                {
                    RealtimeCleanTask* task = new RealtimeCleanTask(_hostid, source);
                    if (task)
                    {
                        _tasks.insert(std::make_pair(source, task));
                    }
                    else
                    {
                        LOG(ERROR) << "create clean task(hostid:" << _hostid << ", source:" << source << ") failed";
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void CleanTaskManager::UpdateDeactiveTask()
{
    while (_running)
    {
        std::queue<RealtimeCleanTask *> tasks;
        std::vector<std::string> sources;
        Maria maria(Configuration::db.url);
        if (maria.QuerySourcesByStatus(_hostid, 0, sources))
        {
            std::lock_guard<std::mutex> lg(_locker);
            for each(std::string source in sources)
            {
                std::map<std::string, RealtimeCleanTask *>::iterator it = _tasks.find(source);
                if (it != _tasks.end())
                {
                    tasks.push(it->second);
                    _tasks.erase(it);
                }
            }
        }

        while (!tasks.empty())
        {
            RealtimeCleanTask* task = tasks.front();
            tasks.pop();

            if (task)
            {
                delete task;
                task = nullptr;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void CleanTaskManager::Startup()
{
    // start main loop
    _running = true;
    _active = std::thread(&CleanTaskManager::UpdateActiveTask, this);
    _deactive = std::thread(&CleanTaskManager::UpdateDeactiveTask, this);
}

void CleanTaskManager::Destroy()
{
    _running = false;

    if (_active.joinable())
    {
        _active.join();
    }

    if (_deactive.joinable())
    {
        _deactive.join();
    }

    for (std::map<std::string, RealtimeCleanTask *>::iterator it = _tasks.begin(); it != _tasks.end(); ++it)
    {
        delete it->second;
        it->second = nullptr;
    }
    _tasks.clear();
}

