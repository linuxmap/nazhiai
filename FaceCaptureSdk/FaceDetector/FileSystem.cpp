
#include "FileSystem.h"

#include <algorithm>
#include <sstream>

#include <time.h>

char* FileSystem::FORMAT_DEFAULT = "yyyy-mm-dd hh:mi:ss.ms";

void FileSystem::ListDir(const std::string& path, FNodeVector& dirs)
{
    _finddata_t findfile;
    intptr_t handle = _findfirst((FixPathWithSlash(path) + "*.*").c_str(), &findfile);
    if (handle == -1) return;

    do
    {
        if (findfile.attrib & _A_SUBDIR)
        {
            if (strcmp(findfile.name, ".") == 0 || strcmp(findfile.name, "..") == 0)
                continue;

            dirs.push_back(FNode(findfile));
        }
    } while (_findnext(handle, &findfile) == 0);

    _findclose(handle);
}

void FileSystem::ListFile(const std::string& path, FNodeVector& files)
{
    _finddata_t findfile;
    intptr_t handle = _findfirst((FixPathWithSlash(path) + "*.*").c_str(), &findfile);
    if (handle == -1) return;

    do
    {
        if ((findfile.attrib & _A_SUBDIR) == 0)
        {
            files.push_back(FNode(findfile));
        }
    } while (_findnext(handle, &findfile) == 0);

    _findclose(handle);
}

void FileSystem::ListAll(const std::string& path, FNodeVector& all)
{
    _finddata_t findfile;
    intptr_t handle = _findfirst((FixPathWithSlash(path) + "*.*").c_str(), &findfile);
    if (handle == -1) return;

    do
    {
        if (findfile.attrib & _A_SUBDIR)
        {
            if (strcmp(findfile.name, ".") == 0 || strcmp(findfile.name, "..") == 0)
                continue;
        }
        all.push_back(FNode(findfile));
    } while (_findnext(handle, &findfile) == 0);

    _findclose(handle);
}

void FileSystem::OrderByName(FNodeVector& nodes, bool asc /*= true*/)
{
    if (asc)
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return strcmp(f.info.name, s.info.name) < 0; });
    } 
    else
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return !(strcmp(f.info.name, s.info.name) < 0); });
    }
}

void FileSystem::OrderByModifyTime(FNodeVector& nodes, bool asc /*= true*/)
{
    if (asc)
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return f.info.time_write < s.info.time_write; });
    }
    else
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return !(f.info.time_write < s.info.time_write); });
    }
}

void FileSystem::OrderByCreateTime(FNodeVector& nodes, bool asc /*= true*/)
{
    if (asc)
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return f.info.time_create < s.info.time_create; });
    }
    else
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return !(f.info.time_create < s.info.time_create); });
    }
}

void FileSystem::OrderByAccessTime(FNodeVector& nodes, bool asc /*= true*/)
{
    if (asc)
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return f.info.time_access < s.info.time_access; });
    }
    else
    {
        std::sort(nodes.begin(), nodes.end(), [](const FNode& f, const FNode& s){ return !(f.info.time_access < s.info.time_access); });
    }
}

bool FileSystem::Exist(const std::string& file)
{
    return _access(file.c_str(), 0) == 0;
}

bool FileSystem::MakeDir(const std::string& path)
{
    return _mkdir(path.c_str()) == 0;
}

bool FileSystem::RemoveDir(const std::string& path)
{
    return _rmdir(path.c_str()) == 0;
}

bool FileSystem::Accessible(const std::string& file, int mode)
{
    return _access(file.c_str(), mode) == 0;
}

void FileSystem::GenerateName(const std::string& format, std::string& name, int timezone/* = 8*/)
{
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME tnow;
    GetSystemTime(&tnow);
#else
    struct tm tnow;
    asctime(&tnow);
#endif

    size_t fsize = format.size();
    std::stringstream ss;
    for (size_t i = 0; i < fsize;)
    {
        if (i + 3 < fsize
            && format[i] == 'y'
            && format[++i] == 'y'
            && format[++i] == 'y'
            && format[++i] == 'y')
        {
            ss << tnow.wYear;
        }
        else if (i + 1 < fsize
            && format[i] == 'm'
            && format[i+1] == 'm')
        {
            if (tnow.wMonth <= 9)
            {
                ss << "0";
            }
            i += 2;
            ss << tnow.wMonth;
        }
        else if (i + 1 < fsize
            && format[i] == 'd'
            && format[i + 1] == 'd')
        {
            if (tnow.wDay <= 9)
            {
                ss << "0";
            }
            i += 2;
            ss << tnow.wDay;
        }
        else if (i + 1 < fsize
            && format[i] == 'h'
            && format[i + 1] == 'h')
        {
            if (tnow.wHour <= 9)
            {
                ss << "0";
            }
            i += 2;
            tnow.wHour += timezone;
            ss << tnow.wHour;
        }
        else if (i + 1 < fsize
            && format[i] == 'H'
            && format[i + 1] == 'H')
        {
            if (tnow.wHour <= 9)
            {
                ss << "0";
            }
            i += 2;
            ss << tnow.wHour;
        }
        else if (i + 1 < fsize
            && format[i] == 'm'
            && format[i + 1] == 'i')
        {
            if (tnow.wMinute <= 9)
            {
                ss << "0";
            }
            i += 2;
            ss << tnow.wMinute;
        }
        else if (i + 1 < fsize
            && format[i] == 's'
            && format[i + 1] == 's')
        {
            if (tnow.wSecond <= 9)
            {
                ss << "0";
            }
            i += 2;
            ss << tnow.wSecond;
        }
        else if (i + 1 < fsize
            && format[i] == 'm'
            && format[i + 1] == 's')
        {
            i += 2;
            ss << tnow.wMilliseconds;
        }
        else
        {
            ss << format[i++];
        }
    }
    name = ss.str();
}

std::string FileSystem::FixPathWithSlash(const std::string& path)
{
    if (path.size() <= 0)
    {
        return "./";
    }
    else
    {
        char ch = path[path.size() - 1];
        if (ch != '\\' && ch != '/')
        {
            return path + "/";
        }
        return path;
    }
}

