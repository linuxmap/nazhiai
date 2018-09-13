
#ifndef _OPERATIONSERVICE_HEADER_H_
#define _OPERATIONSERVICE_HEADER_H_

#include "Operation.h"

class OperationService : public stream::Operation
{
public:
    OperationService();
    ~OperationService();

    virtual bool Play(const stream::ParamPtr& param, ::std::string& result, const ::Ice::Current& current = ::Ice::emptyCurrent) override;
    virtual void Stop(const stream::ParamPtr& param, const ::Ice::Current& current = ::Ice::emptyCurrent) override;

private:
    OperationService(const OperationService&);
    OperationService& operator=(const OperationService&);
};

#endif

