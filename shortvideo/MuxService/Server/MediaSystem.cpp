#include "MediaSystem.h"

#include "FileSystem.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include <sstream>

bool MediaSystem::NavigateToNearestPath(const std::string& root, const std::string& host, int filelimit, MFS& mfs)
{
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME tnow;
    GetSystemTime(&tnow);
#else
    struct tm tnow;
    asctime(&tnow);
#endif

    std::stringstream ss;
    ss << root << host << "/";

    mfs.hpath = ss.str();
    if (!FileSystem::Accessible(mfs.hpath, _A_NORMAL))
    {
        if (!FileSystem::MakeDir(mfs.hpath))
        {
            LOG(ERROR) << "create directory failed: " << mfs.hpath;
            return false;
        }
    }

    ss << tnow.wYear << "/";
    mfs.year = tnow.wYear;
    mfs.ypath = ss.str();
    if (!FileSystem::Accessible(mfs.ypath, _A_NORMAL))
    {
        if (!FileSystem::MakeDir(mfs.ypath))
        {
            LOG(ERROR) << "create directory failed: " << mfs.ypath;
            return false;
        }
    }

    ss << tnow.wMonth << "/";
    mfs.month = tnow.wMonth;
    mfs.mpath = ss.str();
    if (!FileSystem::Accessible(mfs.mpath, _A_NORMAL))
    {
        if (!FileSystem::MakeDir(mfs.mpath))
        {
            LOG(ERROR) << "create directory failed: " << mfs.mpath;
            return false;
        }
    }

    ss << tnow.wDay << "/";
    mfs.day = tnow.wDay;
    mfs.dpath = ss.str();
    if (!FileSystem::Accessible(mfs.dpath, _A_NORMAL))
    {
        mfs.seq = 1;
        if (!FileSystem::MakeDir(mfs.dpath))
        {
            LOG(ERROR) << "create directory failed: " << mfs.dpath;
            return false;
        }
    }
    else
    {
        FileSystem::FNodeVector dirs;
        FileSystem::ListDir(mfs.dpath, dirs);

        mfs.seq =(int) dirs.size();
    }

    if (mfs.seq <= 0)
    {
        mfs.seq = 1;
    }

    ss << mfs.seq << "/";
    mfs.spath = ss.str();
    if (!FileSystem::Accessible(mfs.spath, _A_NORMAL))
    {
        if (!FileSystem::MakeDir(mfs.spath))
        {
            LOG(ERROR) << "create directory failed: " << mfs.spath;
            return false;
        }
    }
    else
    {
        FileSystem::FNodeVector files;
        FileSystem::ListDir(mfs.spath, files);

        mfs.files = (int)files.size();
        if (mfs.files >= filelimit)
        {
            mfs.seq++;
            mfs.files = 0;

            ss.clear();
            ss.str("");
            ss << mfs.dpath << mfs.seq << "/";
            mfs.spath = ss.str();
            if (!FileSystem::Accessible(mfs.spath, _A_NORMAL))
            {
                if (!FileSystem::MakeDir(mfs.spath))
                {
                    LOG(ERROR) << "create directory failed: " << mfs.spath;
                    return false;
                }
            }
        }
    }

    return true;
}

bool MediaSystem::NextMediaPath(int filelimit, MFS& mfs)
{
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME tnow;
    GetSystemTime(&tnow);
#else
    struct tm tnow;
    asctime(&tnow);
#endif

    bool changed = false;
    std::stringstream ss;

    // year changed
    if (tnow.wYear != mfs.year)
    {
        mfs.files = 0;
        changed = true;
        mfs.year = tnow.wYear;
        ss << mfs.hpath << tnow.wYear << "/";
        mfs.ypath = ss.str();

        if (!FileSystem::Accessible(mfs.ypath, _A_NORMAL))
        {
            if (!FileSystem::MakeDir(mfs.ypath))
            {
                LOG(ERROR) << "create directory failed: " << mfs.ypath;
                return false;
            }
        }
    }

    if (changed || mfs.month != tnow.wMonth)
    {
        ss.clear();
        ss.str("");

        mfs.files = 0;
        changed = true;
        mfs.month = tnow.wMonth;
        ss << mfs.ypath << tnow.wMonth << "/";
        mfs.mpath = ss.str();

        if (!FileSystem::Accessible(mfs.mpath, _A_NORMAL))
        {
            if (!FileSystem::MakeDir(mfs.mpath))
            {
                LOG(ERROR) << "create directory failed: " << mfs.mpath;
                return false;
            }
        }
    }

    if (changed || tnow.wDay != mfs.day)
    {
        ss.clear();
        ss.str("");

        mfs.files = 0;
        mfs.day = tnow.wDay;
        ss << mfs.mpath << tnow.wDay << "/";
        mfs.dpath = ss.str();
        mfs.seq = 1;
        if (!FileSystem::Accessible(mfs.dpath, _A_NORMAL))
        {
            if (!FileSystem::MakeDir(mfs.dpath))
            {
                LOG(ERROR) << "create directory failed: " << mfs.dpath;
                return false;
            }
        }
        else
        {
            FileSystem::FNodeVector dirs;
            FileSystem::ListDir(mfs.dpath, dirs);
            mfs.seq = (int)dirs.size();
        }

        if (mfs.seq <= 0)
        {
            mfs.seq = 1;
        }

        ss << mfs.seq << "/";
        mfs.spath = ss.str();
        if (!FileSystem::Accessible(mfs.spath, _A_NORMAL))
        {
            if (!FileSystem::MakeDir(mfs.spath))
            {
                LOG(ERROR) << "create directory failed: " << mfs.spath;
                return false;
            }
        }
        else
        {
            FileSystem::FNodeVector files;
            FileSystem::ListFile(mfs.spath, files);
            mfs.files = (int)files.size();
        }
    }
    
    if (++mfs.files > filelimit)
    {
        mfs.seq++;
        mfs.files = 1;

        ss.clear();
        ss.str("");
        ss << mfs.dpath << mfs.seq << "/";
        mfs.spath = ss.str();
        if (!FileSystem::Accessible(mfs.spath, _A_NORMAL))
        {
            if (!FileSystem::MakeDir(mfs.spath))
            {
                LOG(ERROR) << "create directory failed: " << mfs.spath;
                return false;
            }
        }
    }

#ifdef _DEVELOP_DEBUG
    printf("sequence: %d, files: %d\n", mfs.seq, mfs.files);
#endif

    return true;
}

