
#ifndef _MEDIASYSTEM_HEADER_H_
#define _MEDIASYSTEM_HEADER_H_

#include <string>

class MediaSystem
{
public:
    struct MFS
    {
        std::string hpath = {};
        int year = 0;
        std::string ypath = {};
        int month = 0;
        std::string mpath = {};
        int day = 0;
        std::string dpath = {};
        int seq = 0;
        std::string spath = {};
        int files = 0;
    };

public:
    static bool NavigateToNearestPath(const std::string& root, const std::string& host, int filelimit, MFS& mfs);
    
    static bool NextMediaPath(int filelimit, MFS& mfs);

private:
    MediaSystem();
    MediaSystem(const MediaSystem&);
    MediaSystem& operator=(const MediaSystem&);
};

#endif

