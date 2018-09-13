#include "ProxyManager.h"

#include <Ice/Ice.h>

//std::mutex ProxyManager::_locker;
SDBusiness::ISDRecordCheckerPrx ProxyManager::_proxy;

bool ProxyManager::Initialize(int argc, char* argv[], std::string& err)
{
    std::stringstream ss;
    try
    {
        static Ice::CommunicatorHolder ich(argc, argv, "conf/SDRecordChecker.client");

        _proxy = SDBusiness::ISDRecordCheckerPrx::checkedCast(
            ich.communicator()->propertyToProxy("RecordChecker.Proxy")->ice_twoway()->ice_secure(false));
        return true;
    }
    catch (Ice::Exception &ex)
    {
        ss << "connect to face server failed: " << ex.what();
        _proxy = nullptr;
    }
    catch (...)
    {
        ss << "connect to face server failed: unknown reason";
        _proxy = nullptr;
    }
    ss >> err;
    return false;
}

SDBusiness::ISDRecordCheckerPrx& ProxyManager::Proxy()
{
    return _proxy;
}
