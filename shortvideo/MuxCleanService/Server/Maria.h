
#ifndef _MARIA_HEADER_H_
#define _MARIA_HEADER_H_

#include <soci.h>

struct VideoItem 
{
    long long generatedpoint = 0;
    long long stoppedpoint = 0;
    std::string path = {};
};

struct FailedItem
{
    int hostid = 0;
    std::string source = {};
    long long generatedpoint = 0;
    long long stoppedpoint = 0;
};

class Maria
{
public:
    Maria(const std::string& url);
    ~Maria();

    bool QueryItems(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint, std::vector<VideoItem>& items);
    bool DeleteItem(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint);

    bool QuerySourcesByStatus(int hostId, int status, std::vector<std::string>& sources);

    bool QueryActiveSourceLastCleanTimepoint(int hostId, const std::string& source, long long& timepoint);
    bool UpdateSourceLastCleanTimepoint(int hostId, const std::string& source, long long timepoint);

    bool OneHost(int hostId, std::string& host, std::string& root);

    bool QueryFailedItems(std::vector<FailedItem>& failedItems);
    void ManageFailedItem(int hostId, const std::string& source, long long generatedpoint, long long stoppedpoint, bool add);

private:
    void LogError(const std::string& err);

private:
    soci::session _sql;
    std::string _err;
    std::stringstream _format;

private:
    Maria();
    Maria(const Maria&);
    Maria& operator=(const Maria&);
};

#endif

