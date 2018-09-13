#include "FailedCleanTask.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "Configuration.h"

#include "Maria.h"
#include "FileSystem.h"

#include "ProxyManager.h"

FailedCleanTask::FailedCleanTask()
    : _cleaning(false), _task(), _locker(), _condition()
{
    _cleaning = true;
    _task = std::thread(&FailedCleanTask::Clean, this);
}

FailedCleanTask::~FailedCleanTask()
{
    _cleaning = false; 
    _condition.notify_one();
    if (_task.joinable())
    {
        _task.join();
    }
}

void FailedCleanTask::Clean()
{
    while (_cleaning)
    {
        std::vector<FailedItem> failedItems;
        Maria maria(Configuration::db.url);
        maria.QueryFailedItems(failedItems);

        for (size_t idx = 0; idx < failedItems.size() && _cleaning; ++idx)
        {
            FailedItem& item = failedItems[idx];
            CleanShortVideos(maria, item.hostid, item.source, item.generatedpoint, item.stoppedpoint);
        }

        std::unique_lock<std::mutex> ul(_locker);
        _condition.wait_for(ul, std::chrono::seconds(1), [this]() { return !_cleaning; });
    }
}

bool FailedCleanTask::QueryFaceTimepoints(int hostid, const std::string& source, long long beg, long long end, std::vector<long long>& timepoints)
{
    std::stringstream ss;
    try
    {
        SDBusiness::ISDRecordCheckerPrx& proxy = ProxyManager::Proxy();
        if (proxy)
        {
            ::SDBusiness::SCaptureQueryCond cond;
            cond.videoUrl = source;
            cond.begin = beg;
            cond.end = end;
            proxy->QueryCaptureList(cond, timepoints);
        }
    }
    catch (Ice::Exception &ex)
    {
        ss << "[FailedCleanTask] query face time point information failed: " << ex.what();
    }
    catch (...)
    {
        ss << "[FailedCleanTask] query face time point information failed: unknown reason";
    }

    std::string err = ss.str();
    if (err.empty())
    {
        return true;
    }
    else
    {
        LOG(ERROR) << err;
        return false;
    }
}

void FailedCleanTask::CleanShortVideos(Maria& maria, int hostid, const std::string& source, long long beg, long long end)
{
    std::vector<long long> facetimepoints;
    if (QueryFaceTimepoints(hostid, source, beg, end, facetimepoints))
    {
        // query all short video since last clean time point
        std::vector<VideoItem> videoitems;
        if (maria.QueryItems(hostid, source, beg, end, videoitems))
        {
            size_t itemsize = videoitems.size();
            size_t facesize = facetimepoints.size();
            size_t lastfacepos = 0;
            for (size_t idx = 0; idx < itemsize; ++idx)
            {
                VideoItem& item = videoitems[idx];
                bool foundface = false;
                while (lastfacepos < facesize)
                {
                    long long facetimepoint = facetimepoints[lastfacepos];
                    if (facetimepoint < item.generatedpoint)
                    {
                        lastfacepos++;
                    }
                    else if (facetimepoint >= item.generatedpoint && facetimepoint <= item.stoppedpoint)
                    {
                        foundface = true;
                        break;
                    }
                    else
                    {
                        break;
                    }
                }

                if (!foundface)
                {
                    std::stringstream ss;
                    ss << Configuration::action.root << item.path;
                    if (FileSystem::Accessible(ss.str(), _A_NORMAL))
                    {
                        if (!FileSystem::RemoveFile(ss.str()))
                        {
                            LOG(WARNING) << "[FailedCleanTask] delete short video(" << ss.str() << ") failed";
                        }
                        else
                        {
                            maria.DeleteItem(hostid, source, item.generatedpoint, item.stoppedpoint);
                        }
                    }
                    else
                    {
                        LOG(WARNING) << "[FailedCleanTask] short video(" << ss.str() << ") is not accessible or not exist";
                    }
                }
            }

            maria.ManageFailedItem(hostid, source, beg, end, false);
        }
    }
}

