#include "TriggerCleanTask.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "Configuration.h"

#include "Maria.h"
#include "FileSystem.h"

#include "TriggerCleanTaskManager.h"

#include "ProxyManager.h"

#include <iomanip>

static long long Now(std::chrono::system_clock::time_point& timepoint)
{
    typedef std::chrono::milliseconds time_precision;
    typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
    now_time_point tp = std::chrono::time_point_cast<time_precision>(timepoint);
    return tp.time_since_epoch().count();
}

TriggerCleanTask::TriggerCleanTask()
    : _hostid(0), _source()
    , _begintime(), _endtime()
    , _cleanning(false), _task()
    , _locker(), _condition()
{
    _cleanning = true;
    _task = std::thread(&TriggerCleanTask::Clean, this);
}

TriggerCleanTask::~TriggerCleanTask()
{
    _cleanning = false;
    _condition.notify_one();
    if (_task.joinable())
    {
        _task.join();
    }
}

bool TriggerCleanTask::TaskConflict(int hostid, const std::string& source, const std::string& begin, const std::string& end)
{
    return _hostid == hostid && _source == source && ((begin >= _begintime && begin < _endtime) || (end <= _endtime && end > _begintime));
}

void TriggerCleanTask::Start(int hostid, const std::string& source, const std::string& begintime, const std::string& endtime)
{
    std::lock_guard<std::mutex> lg(_locker);
    _hostid = hostid;
    _source = source;
    _begintime = begintime;
    _endtime = endtime;
}

void TriggerCleanTask::Clean()
{
    while (_cleanning)
    {
        std::unique_lock<std::mutex> ul(_locker);
        _condition.wait_for(ul, std::chrono::seconds(5), [this](){ return !_source.empty(); });

        if (!_cleanning || _source.empty())
        {
            continue;
        }

        std::stringstream ss;
        std::tm tm;

        ss << _begintime;
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        long long begin_timepoint = Now(std::chrono::system_clock::from_time_t(mktime(&tm)));

        ss.clear();
        ss.str("");
        ss << _endtime;
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        long long end_timepoint = Now(std::chrono::system_clock::from_time_t(mktime(&tm)));

        LOG(INFO) << "trigger clean task(begin:" << _begintime << ", end:" << _endtime << "; timepoint:" << begin_timepoint << "~" << end_timepoint << ") begin success";

        long long step = Configuration::action.frequency * 60 * 1000;
        while (begin_timepoint < end_timepoint)
        {
            CleanShortVideos(begin_timepoint, begin_timepoint + step);
            begin_timepoint += step;
        }

        ss << "trigger clean task(begin:" << _begintime << ", end:" << _endtime << "; timepoint:" << begin_timepoint << "~" << end_timepoint << ") end success";

        LOG(INFO) << ss.str();

        _source = "";

        TriggerCleanTaskManager::Free(this);
    }
}

bool TriggerCleanTask::QueryFaceTimepoints(long long beg, long long end, std::vector<long long>& timepoints)
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
        ss << "clean task(begin:" << _begintime << ", end:" << _endtime << "; timepoint:" << beg << "~" << end << ") failed: "
            << "query face time point information failed: " << ex.what();
    }
    catch (...)
    {
        ss << "clean task(begin:" << _begintime << ", end:" << _endtime << "; timepoint:" << beg << "~" << end << ") failed: "
            << "query face time point information failed: unknown reason";
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

void TriggerCleanTask::CleanShortVideos(long long beg, long long end)
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
                            LOG(WARNING) << "delete short video(" << ss.str() << ") failed";
                        }
                        else
                        {
                            maria.DeleteItem(_hostid, _source, item.generatedpoint, item.stoppedpoint);
                        }
                    }
                    else
                    {
                        LOG(WARNING) << "short video(" << ss.str() << ") is not accessible or not exist";
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
}




