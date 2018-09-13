
#ifndef _PROXYMANAGER_HEADER_H_
#define _PROXYMANAGER_HEADER_H_

#include "IceSDRecordChecker.h"

//#include <mutex>

class ProxyManager
{
public:

    static bool Initialize(int argc, char* argv[], std::string& err);
    static SDBusiness::ISDRecordCheckerPrx& Proxy();

private:
    //static std::mutex _locker;
    static SDBusiness::ISDRecordCheckerPrx _proxy;

private:
    ProxyManager();
    ProxyManager(const ProxyManager&);
    ProxyManager& operator=(const ProxyManager&);
};

#endif

