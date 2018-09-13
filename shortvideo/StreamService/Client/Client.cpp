// Client.cpp : 定义控制台应用程序的入口点。
//

#include <Ice/Ice.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#ifdef NDEBUG
#pragma comment(lib, "glog.lib")
#else
#pragma comment(lib, "glogd.lib")
#endif

#include "Operation.h"

using namespace stream;

static void glog_CrashWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

static void glog_Initialize()
{
    google::InitGoogleLogging("StreamClient");
    google::SetStderrLogging(google::GLOG_FATAL);
    FLAGS_log_dir = "logs";
    FLAGS_colorlogtostderr = true;  // Set log color  
    FLAGS_logbufsecs = 0;  // Set log output speed(s)  
    FLAGS_max_log_size = 128;  // Set max log file size  
    FLAGS_stop_logging_if_full_disk = true;  // If disk is full  
    google::InstallFailureWriter(&glog_CrashWriter);
    google::InstallFailureSignalHandler();
}

int main(int argc, char* argv[])
{
#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL();
    Ice::registerIceUDP();
    Ice::registerIceWS();
#endif

    int status = 0;

    try
    {
        //
        // configure google logger
        //
        glog_Initialize();

        //
        // The communicator initialization removes all Ice-related arguments from argc/argv
        //
        if (argc > 1)
        {
            LOG(FATAL) << "too many arguments";
            return -1;
        }

        //
        // CommunicatorHolder's ctor initializes an Ice communicator,
        // and its dtor destroys this communicator.
        //
        Ice::CommunicatorHolder ich(argc, argv, "conf/config.client");

        OperationPrx twoway = OperationPrx::checkedCast(
            ich.communicator()->propertyToProxy("Stream.Proxy")->ice_twoway()->ice_secure(false));
        if (!twoway)
        {
            LOG(FATAL) << "invalid proxy";
            return -1;
        }

        ParamPtr param(new Param());
        param->file = "192.168.2.64/2018/8/23/1/189768765.avi";
        std::string result;
        
        twoway->Play(param, result);

        printf("result: %s\n", result.c_str());

        system("pause");

    }
    catch (const std::exception& ex)
    {
        LOG(FATAL) << ex.what();
        status = -1;
    }

    return status;
}

