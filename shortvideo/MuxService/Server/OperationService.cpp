#include "OperationService.h"

#include "Maria.h"
#include "TaskManager.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

OperationService::OperationService()
{
}

OperationService::~OperationService()
{
}

bool OperationService::Start(const mux::ParamPtr& param, ::std::string& result, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    return TaskManager::StartTask(param->url, result);
}

void OperationService::Stop(const mux::ParamPtr& param, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    TaskManager::StopTask(param->url);
}

bool OperationService::One(const ::std::string& url, const ::std::string& timepoint, ::std::string& path, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    if (url.empty() || timepoint.empty())
    {
        path = "url or timepoint is empty";
        return false;
    }

    Maria maria(TaskManager::db.url);
    bool res = maria.OneItem(url, timepoint, path);
    if (res)
    {
        if (TaskManager::fs.web_service)
        {
            path = TaskManager::fs.web + path;
        }
    }
    return res;
}

