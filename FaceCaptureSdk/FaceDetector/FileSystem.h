#ifndef _FILESYSTEM_HEADER_H_
#define _FILESYSTEM_HEADER_H_


#include <windows.h>

#include <iostream>
#include <io.h>
#include <direct.h>
#include <vector>

class FileSystem
{
public:
    static char* FORMAT_DEFAULT;

public:
    struct FNode {
        _finddata_t info;

        FNode()
        {
            memset(&info, 0, sizeof(info));
        }

        explicit FNode(const _finddata_t& fdata)
        {
            memcpy(&info, &fdata, sizeof(info));
        }

        FNode(const FNode& rhs)
        {
            memcpy(&info, &rhs.info, sizeof(info));
        }

        FNode& operator=(const FNode& rhs)
        {
            if (this != &rhs)
            {
                memcpy(&info, &rhs.info, sizeof(info));
            }
            return *this;
        }
    };

    typedef std::vector<FNode> FNodeVector;

public:
    static void ListDir(const std::string& path, FNodeVector& dirs);
    static void ListFile(const std::string& path, FNodeVector& files);
    static void ListAll(const std::string& path, FNodeVector& all);

    static void OrderByName(FNodeVector& nodes, bool asc = true);
    static void OrderByModifyTime(FNodeVector& nodes, bool asc = true);
    static void OrderByCreateTime(FNodeVector& nodes, bool asc = true);
    static void OrderByAccessTime(FNodeVector& nodes, bool asc = true);

    static bool Exist(const std::string& file);
    static bool MakeDir(const std::string& path);
    static bool RemoveDir(const std::string& path);
    static bool Accessible(const std::string& file, int mode);

    static void GenerateName(const std::string& format, std::string& name, int timezone = 8);
    
private:
    static std::string FixPathWithSlash(const std::string& path);

private:
    FileSystem();
    FileSystem(const FileSystem&);
    FileSystem& operator=(const FileSystem&);
};

#endif


