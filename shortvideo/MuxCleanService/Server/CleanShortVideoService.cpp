#include "CleanShortVideoService.h"

#include "TriggerCleanTaskManager.h"

CleanShortVideoService::CleanShortVideoService()
    : _failedCleanTask()
{
}

CleanShortVideoService::~CleanShortVideoService()
{
    TriggerCleanTaskManager::Destroy();
}

bool CleanShortVideoService::Clean(const clean::ParamPtr& param, std::string& result, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    return TriggerCleanTaskManager::AddCustomCleanTask(param->hostid, param->url, param->begin, param->end, result);
}

