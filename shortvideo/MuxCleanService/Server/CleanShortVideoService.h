
#ifndef _CLEANSHORTVIDEOSERVICE_HEADER_H_
#define _CLEANSHORTVIDEOSERVICE_HEADER_H_

#include "Operation.h"

#include "FailedCleanTask.h"

class CleanShortVideoService : public clean::CleanShortVideo
{
public:
    CleanShortVideoService();
    ~CleanShortVideoService();

    virtual bool Clean(const clean::ParamPtr& param, std::string& result, const ::Ice::Current& current = ::Ice::emptyCurrent) override;

private:
    FailedCleanTask _failedCleanTask;

private:
    CleanShortVideoService(const CleanShortVideoService&);
    CleanShortVideoService& operator=(const CleanShortVideoService&);
};

#endif

