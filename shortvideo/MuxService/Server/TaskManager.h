
#ifndef _TASKMANAGER_HEADER_H_
#define _TASKMANAGER_HEADER_H_

#include "RtspStreamMuxTask.h"

#include <map>

class TaskManager
{
public:
    static bool Startup(std::string& err);
    static bool StartTask(const std::string& url, std::string& result);
    static void StopTask(const std::string& url);
    static void Shutdown();

public:
    struct FileInfo 
    {
        std::string host = {};
        std::string root = {};
        std::string web = {};
        bool web_service = false;
        int max_files = 500;
        int min_duration = 10;
        std::string format = {};
    };
    static FileInfo fs;

    struct DbInfo
    {
        std::string url = {};
        std::string username = {};
        std::string password = {};
    };

    static DbInfo db;

private:
    static std::mutex _locker;
    static std::map<std::string, RtspStreamMuxTask*> _tasks;

private:
    TaskManager();
    TaskManager(const TaskManager&);
    TaskManager& operator=(const TaskManager&);
};

#endif

