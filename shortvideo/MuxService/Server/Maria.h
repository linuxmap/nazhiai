
#ifndef _MARIA_HEADER_H_
#define _MARIA_HEADER_H_

#include <soci.h>

#include <sstream>

class Maria
{
public:
    Maria(const std::string& url);
    ~Maria();

    bool OneItem(const std::string& source, const std::string& timepoint, std::string& path);
    bool OneItem(int hostId, const std::string& source, long long generated, long long stopped, const std::string& path, std::string& result);

    bool OneHost(const std::string& ip, const std::string& root, std::string& result);
    bool OneHost(const std::string& ip, const std::string& root, int& hostId, std::string& result);

    void ActivateSouce(int hostId, const std::string& source, long long last_timepoint);
    void DeactivateSouce(int hostId, const std::string& source);

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

