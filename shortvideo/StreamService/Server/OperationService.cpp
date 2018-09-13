#include "OperationService.h"

#include "PusherManager.h"

OperationService::OperationService()
{
}

OperationService::~OperationService()
{
}

bool OperationService::Play(const stream::ParamPtr& param, ::std::string& result, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    return PusherManager::PushStream(param->file, result);
}

void OperationService::Stop(const stream::ParamPtr& param, const ::Ice::Current& current /*= ::Ice::emptyCurrent*/)
{
    
}

