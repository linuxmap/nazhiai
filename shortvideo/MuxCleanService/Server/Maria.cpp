#include "Maria.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

Maria::Maria(const std::string& url)
    : _sql(), _err(), _format()
{
    try 
    {
        _sql.open(url);
    }
    catch (std::exception const &e)
    {
        _format << "open mysql connection failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
}

Maria::~Maria()
{
    try
    {
        _sql.close();
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "close mysql connection failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
}

bool Maria::QueryItems(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint, std::vector<VideoItem>& items)
{
    try
    {
        VideoItem item;
        soci::statement st = (
            _sql.prepare <<
            "select generated_timepoint, stopped_timepoint, path from short_video_item where source = :source and host_id = :hostId and generated_timepoint >= :generatedpoint and stopped_timepoint <= :stoppedpoint order by generated_timepoint",
            soci::into(item.generatedpoint),
            soci::into(item.stoppedpoint),
            soci::into(item.path),
            soci::use(source),
            soci::use(hostId),
            soci::use(generatedpoint),
            soci::use(stoppedpoint));
        st.execute();
        while (st.fetch())
        {
            items.push_back(item);
        }
        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query short video file(host id:" << hostId << ", url:" << source << ", generated timepoint:" << generatedpoint << ", stopped timepoint:" << stoppedpoint << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::DeleteItem(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint)
{
    try
    {
        _sql.once <<
            "delete from short_video_item where source = :source and host_id = :hostId and generated_timepoint = :generatedpoint and stopped_timepoint = :stoppedpoint",
            soci::use(source),
            soci::use(hostId),
            soci::use(generatedpoint),
            soci::use(stoppedpoint);
        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "delete short video file(host id:" << hostId << ", url:" << source << ", generated timepoint:" << generatedpoint << ", stopped timepoint:" << stoppedpoint << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::QuerySourcesByStatus(int hostId, int status, std::vector<std::string>& sources)
{
    try
    {
        std::string source;
        soci::statement st = (
            _sql.prepare <<
            "select source from short_video_active_source where host_id = :hostId and status = :status",
            soci::into(source),
            soci::use(hostId),
            soci::use(status));
        st.execute();
        while (st.fetch())
        {
            sources.push_back(source);
        }
        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query active short video source(host id:" << hostId << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::QueryActiveSourceLastCleanTimepoint(int hostId, const std::string& source, long long& timepoint)
{
    try
    {
        _sql.once <<
            "select last_clean_timepoint from short_video_active_source where host_id = :hostId and source = :source",
            soci::into(timepoint),
            soci::use(hostId),
            soci::use(source);

        return _sql.got_data();
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query active short video source(host id:" << hostId << ", source: " << source << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::UpdateSourceLastCleanTimepoint(int hostId, const std::string& source, long long timepoint)
{
    try
    {
        _sql.once <<
            "update short_video_active_source set last_clean_timepoint = :last_clean_timepoint where host_id = :hostId and source = :source",
            soci::use(timepoint),
            soci::use(hostId),
            soci::use(source);

        return _sql.got_data();
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query active short video source(host id:" << hostId << ", source: " << source << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::OneHost(int hostId, std::string& host, std::string& root)
{
    try
    {
        _sql.once <<
            "select ip, root from short_video_host where id = :hostid limit 1",
            soci::into(host),
            soci::into(root),
            soci::use(hostId);
        return _sql.got_data();
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query short video host(id:" << hostId << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

bool Maria::QueryFailedItems(std::vector<FailedItem>& failedItems)
{
    try
    {
        FailedItem item;
        soci::statement st = (
            _sql.prepare <<
            "select host_id, source, begin_timepoint, end_timepoint from short_video_clean_failed limit 10",
            soci::into(item.hostid),
            soci::into(item.source),
            soci::into(item.generatedpoint),
            soci::into(item.stoppedpoint));
        st.execute();
        while (st.fetch())
        {
            failedItems.push_back(item);
        }
        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query short video clean failed item failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }
    LogError(_err);
    return false;
}

void Maria::ManageFailedItem(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint, bool add)
{
    try
    {
        if (add)
        {
            _sql.once <<
                "select 1 from short_video_clean_failed where host_id = :host_id and source = :source and begin_timepoint = :begin_timepoint and end_timepoint = :end_timepoint",
                soci::use(hostId),
                soci::use(source),
                soci::use(generatedpoint),
                soci::use(stoppedpoint);
            if (!_sql.got_data())
            {
                _sql.once <<
                    "insert into short_video_clean_failed(host_id, source, begin_timepoint, end_timepoint) values (:host_id, :source, :begin_timepoint, :end_timepoint)",
                    soci::use(hostId),
                    soci::use(source),
                    soci::use(generatedpoint),
                    soci::use(stoppedpoint);
            }
        }
        else
        {
            _sql.once <<
                "delete from short_video_clean_failed where host_id = :host_id and source = :source and begin_timepoint = :begin_timepoint and end_timepoint = :end_timepoint",
                soci::use(hostId),
                soci::use(source),
                soci::use(generatedpoint),
                soci::use(stoppedpoint);
        }
        return;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        if (add)
        {
            _format << "add";
        } 
        else
        {
            _format << "delete";
        }
        _format << " short video clean failed item(host: " << hostId << ", source: " << source << ", begin timepoint : " << generatedpoint << ", end timepoint : " << stoppedpoint << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }

    LogError(_err);
}

void Maria::LogError(const std::string& err)
{
    if (!err.empty())
    {
        LOG(ERROR) << err;
    }
}

