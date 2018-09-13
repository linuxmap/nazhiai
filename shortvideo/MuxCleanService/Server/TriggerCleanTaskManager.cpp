#include "TriggerCleanTaskManager.h"

#include "TriggerCleanTask.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

std::mutex TriggerCleanTaskManager::_locker;
std::queue<TriggerCleanTask*> TriggerCleanTaskManager::_freetasks;
std::map<TriggerCleanTask*, TriggerCleanTask*> TriggerCleanTaskManager::_customcleantasks;

bool TriggerCleanTaskManager::AddCustomCleanTask(int hostid, const std::string& source, const std::string& begintime, const std::string& endtime, std::string& result)
{
    bool overlaps = false;
    std::lock_guard<std::mutex> lg(_locker);
    for (std::map<TriggerCleanTask*, TriggerCleanTask*>::iterator it = _customcleantasks.begin(); it != _customcleantasks.end(); ++it)
    {
        if (it->second->TaskConflict(hostid, source, begintime, endtime))
        {
            overlaps = true;
            std::stringstream ss;
            ss << "[TriggerCleanTaskManager] trigger task time range(begin:" << begintime << ", end:" << endtime << ") conflict with another running custom task";
            ss >> result;
            LOG(ERROR) << result;
            break;
        }
    }

    if (!overlaps)
    {
        TriggerCleanTask* task = nullptr;
        if (_freetasks.size() > 0)
        {
            task = _freetasks.front();
            _freetasks.pop();
        } 
        else
        {
            task = new TriggerCleanTask();
        }
        
        if (task)
        {
            task->Start(hostid, source, begintime, endtime);
            _customcleantasks.insert(std::make_pair(task, task));
        }
        else
        {
            std::stringstream ss;
            ss << "[TriggerCleanTaskManager] create trigger task time range(begin:" << begintime << ", end:" << endtime << ") failed";
            ss >> result;
            LOG(ERROR) << result;
        }
    }

    return !overlaps;
}

void TriggerCleanTaskManager::Free(TriggerCleanTask* task)
{
    if (task)
    {
        std::lock_guard<std::mutex> lg(_locker);
        std::map<TriggerCleanTask *, TriggerCleanTask *>::iterator it = _customcleantasks.find(task);
        if (it != _customcleantasks.end())
        {
            _customcleantasks.erase(it);
        }
        _freetasks.push(task);
    }
}

void TriggerCleanTaskManager::Destroy()
{
    std::lock_guard<std::mutex> lg(_locker);
    while (!_freetasks.empty())
    {
        TriggerCleanTask* task = _freetasks.front();
        _freetasks.pop();
        delete task;
        task = nullptr;
    }
    for (std::map<TriggerCleanTask *, TriggerCleanTask *>::iterator it = _customcleantasks.begin(); it != _customcleantasks.end(); ++it)
    {
        TriggerCleanTask* task = it->second;
        delete task;
        task = nullptr;
    }
    _customcleantasks.clear();
}

