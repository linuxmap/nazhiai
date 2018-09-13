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
        _format.clear();
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

bool Maria::OneItem(const std::string& source, const std::string& timepoint, std::string& path)
{
    bool error = false;
    try 
    {
        soci::indicator ind;
        _sql.once <<
            "select path from short_video_item where source = :url and :timepoint between generated_timepoint and stopped_timepoint - 1 limit 1",
            soci::into(path, ind),
            soci::use(source),
            soci::use(std::atoll(timepoint.c_str()));
        if (_sql.got_data()) 
        {
            return true;
        }
        _format.clear();
        _format << "no short video found(url:" << source << ", timepoint:" << timepoint << ")";
        path = _format.str();
        _format.str("");
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "query short video file(belong to:" << source << ", generated at: " << timepoint << ") failed: " << e.what();
        path = _format.str();
        _format.str("");
        error = true;
    }
    
    if (error)
    {
        LogError(path);
    }
    return false;
}

bool Maria::OneItem(int hostId, const std::string& source, long long generated, long long stopped, const std::string& path, std::string& result)
{
    try
    {
        _sql.once <<
            "insert into short_video_item(host_id, source, generated_timepoint, stopped_timepoint, path) values (:host_id, :source, :generated_timepoint, :stopped_timepoint, :path)",
            soci::use(hostId),
            soci::use(source),
            soci::use(generated), 
            soci::use(stopped), 
            soci::use(path);

        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "save short video file(" << path <<") failed: " << e.what();
        result = _format.str();
        _format.str("");
    }
    LogError(result);
    return false;
}

bool Maria::OneHost(const std::string& ip, const std::string& root, std::string& result)
{
    try 
    {
        _sql.once <<
            "insert into short_video_host(ip, root) values (:ip, :root)",
            soci::use(ip),
            soci::use(root);
        return true;
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "save short video host(" << ip << ") failed: " << e.what();
        result = _format.str();
        _format.str("");
    }
    LogError(result);
    return false;
}

bool Maria::OneHost(const std::string& ip, const std::string& root, int& hostId, std::string& result)
{
    bool error = false;
    try
    {
        _sql.once <<
            "select id from short_video_host where ip = :url and root = :root limit 1",
            soci::into(hostId),
            soci::use(ip),
            soci::use(root);
        if (_sql.got_data())
        {
            return true;
        }
        _format.clear();
        _format << "no short video host found(ip:" << ip << ", root : " << root << ")";
        result = _format.str();
        _format.str("");
    }
    catch (std::exception const &e)
    {
        error = true;
        _format.clear();
        _format << "query short video host(" << ip << ") failed: " << e.what();
        result = _format.str();
        _format.str("");
    }
    
    if (error)
    {
        LogError(result);
    }
    return false;
}

void Maria::ActivateSouce(int hostId, const std::string& source, long long last_timepoint)
{
    try
    {
        int count = 0;
        _sql.once <<
            "select 1 from short_video_active_source where host_id = :host_id and source = :source",
            soci::into(count),
            soci::use(hostId),
            soci::use(source);
        if (!_sql.got_data())
        {
            _sql.once <<
                "insert into short_video_active_source(host_id, source, status, status_date, last_clean_timepoint) values (:host_id, :source, 1, sysdate(), :last_clean_timepoint)",
                soci::use(hostId),
                soci::use(source),
                soci::use(last_timepoint);
        }
        else
        {
            _sql.once <<
                "update short_video_active_source set status = 1, status_date = sysdate() where host_id = :host_id and source = :source",
                soci::use(hostId),
                soci::use(source);
        }
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "activate short video source(host: " << hostId << ", source: " << source << ") failed: " << e.what();
        _err = _format.str();
        _format.str("");
    }

    LogError(_err);
}

void Maria::DeactivateSouce(int hostId, const std::string& source)
{
    try
    {
        _sql.once <<
            "update short_video_active_source set status = 0, status_date = sysdate() where host_id = :host_id and source = :source",
            soci::use(hostId),
            soci::use(source);
    }
    catch (std::exception const &e)
    {
        _format.clear();
        _format << "deactivate short video source(host: " << hostId << ", source: " << source << ") failed: " << e.what();
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



