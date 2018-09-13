
#ifndef _CONFIGURATION_HEADER_H_
#define _CONFIGURATION_HEADER_H_

#include <string>

class Configuration
{
public:
    static bool Load(std::string& err);
    static void Unload();

public:
    struct ActionInfo
    {
        int hostid = 0;
        std::string host = {};
        std::string root = {};

        int frequency = 60;
        int margin = 5;
    };
    static ActionInfo action;

    struct DbInfo
    {
        std::string url = {};
    };
    static DbInfo db;

private:
    Configuration();
    Configuration(const Configuration&);
    Configuration& operator=(const Configuration&);
};

#endif

