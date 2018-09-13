#include "RealtimeCleanTask.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "Configuration.h"

#include "Maria.h"
#include "FileSystem.h"

#include "ProxyManager.h"

static long long Now()
{
    typedef std::chrono::milliseconds time_precision;
    typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
    now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
    return tp.time_since_epoch().count();
}

RealtimeCleanTask::RealtimeCleanTask(int hostid, const std::string& source)
    : _hostid(hostid), _source(source), _last_clean_timepoint(0)
    , _cleaning(false), _task(), _locker(), _condition()
{
    _cleaning = true;
    _task = std::thread(&RealtimeCleanTask::Clean, this);
}

RealtimeCleanTask::~RealtimeCleanTask()
{
    _cleaning = false; 
    _condition.notify_one();
    if (_task.joinable())
    {
        _task.join();
    }
}

void RealtimeCleanTask::Clean()
{
    long long timepoint_now = Now();
    {
        Maria maria(Configuration::db.url);
        maria.QueryActiveSourceLastCleanTimepoint(_hostid, _source, _last_clean_timepoint);
        
        long long step = Configuration::action.frequency * 60 * 1000;
        long long next = _last_clean_timepoint + step;
        while (next < timepoint_now)
        {
            CleanShortVideos(_last_clean_timepoint, next);
            _last_clean_timepoint = next;
            next += step;
        }
    }

    while (_cleaning)
    {
        if (_cleaning)
        {
            timepoint_now = Now();
            // query data
            CleanShortVideos(_last_clean_timepoint, timepoint_now);
            _last_clean_timepoint = timepoint_now;
        }

        std::unique_lock<std::mutex> ul(_locker);
        _condition.wait_for(ul, std::chrono::minutes(Configuration::action.frequency), [this]() { return !_cleaning; });
    }
}

bool RealtimeCleanTask::QueryFaceTimepoints(long long beg, long long end, std::vector<long long>& timepoints)
{
    std::stringstream ss;
    try
    {
        SDBusiness::ISDRecordCheckerPrx& proxy = ProxyManager::Proxy();
        if (proxy)
        {
            ::SDBusiness::SCaptureQueryCond cond;
            cond.videoUrl = _source;
            cond.begin = beg;
            cond.end = end;
            proxy->QueryCaptureList(cond, timepoints);
        }
    }
    catch (Ice::Exception &ex)
    {
        ss << "[RealtimeCleanTask] query face time point information failed: " << ex.what();
    }
    catch (...)
    {
        ss << "[RealtimeCleanTask] query face time point information failed: unknown reason";
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

void RealtimeCleanTask::CleanShortVideos(long long beg, long long end)
{
    Maria maria(Configuration::db.url);

    std::vector<long long> facetimepoints;
    if (QueryFaceTimepoints(beg, end, facetimepoints))
    {
        // query all short video since last clean time point
        std::vector<VideoItem> videoitems;
        if (maria.QueryItems(_hostid, _source, beg, end, videoitems))
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
                            LOG(WARNING) << "[RealtimeCleanTask] delete short video(" << ss.str() << ") failed";
                        }
                        else
                        {
                            maria.DeleteItem(_hostid, _source, item.generatedpoint, item.stoppedpoint);
                            //LOG(INFO) << "delete short video(" << ss.str() << ") success";
                        }
                    }
                    else
                    {
                        LOG(WARNING) << "[RealtimeCleanTask] short video(" << ss.str() << ") is not accessible or not exist";
                    }
                }
            }
        }
        else
        {
            // save failed time point
            maria.ManageFailedItem(_hostid, _source, beg, end, true);
        }
    }
    else
    {
        // save failed time point
        maria.ManageFailedItem(_hostid, _source, beg, end, true);
    }

    // update last clean time point
    maria.UpdateSourceLastCleanTimepoint(_hostid, _source, end);
}

