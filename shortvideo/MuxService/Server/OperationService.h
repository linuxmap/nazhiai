
#ifndef _OPERATIONSERVICE_HEADER_H_
#define _OPERATIONSERVICE_HEADER_H_

#include "Operation.h"

class OperationService : public mux::Operation
{
public:
    OperationService();
    ~OperationService();

    virtual bool Start(const mux::ParamPtr& param, ::std::string& result, const ::Ice::Current& current = ::Ice::emptyCurrent) override;
    virtual void Stop(const mux::ParamPtr& param, const ::Ice::Current& current = ::Ice::emptyCurrent) override;

    virtual bool One(const ::std::string& url, const ::std::string& timepoint, ::std::string& path, const ::Ice::Current& current = ::Ice::emptyCurrent) override;

private:
    OperationService(const OperationService&);
    OperationService& operator=(const OperationService&);
};

#endif

