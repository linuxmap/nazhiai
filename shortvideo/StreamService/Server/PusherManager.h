
#ifndef _PUSHERMANAGER_HEADER_H_
#define _PUSHERMANAGER_HEADER_H_

#include "RtmpPusher.h"

#include <map>

class PusherManager
{
public:
    static bool Startup(std::string& err);
    static bool PushStream(const std::string& file, std::string& result);
    static void Shutdown();

public:
    struct MediaInfo 
    {
        std::string root;
        std::string rtmp;
    };
    static MediaInfo md;

    struct DbInfo
    {
        std::string url;
        std::string username;
        std::string password;
    };

    static DbInfo db;

private:
    static RtmpPusher* Alloc();
    static void Free(RtmpPusher* pusher);

    friend class RtmpPusher;
private:
    static std::mutex _locker;
    static std::queue<RtmpPusher*> _free_pushers;

private:
    PusherManager();
    PusherManager(const PusherManager&);
    PusherManager& operator=(const PusherManager&);
};

#endif

