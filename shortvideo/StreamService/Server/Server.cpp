// Server.cpp : 定义控制台应用程序的入口点。
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

#include "OperationService.h"

#include "PusherManager.h"

static void glog_CrashWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

static void glog_Initialize()
{
    google::InitGoogleLogging("StreamService");
    google::SetStderrLogging(google::GLOG_FATAL);
    FLAGS_log_dir = "logs";
    FLAGS_colorlogtostderr = true;  // Set log color  
    FLAGS_logbufsecs = 0;  // Set log output speed(s)  
    FLAGS_max_log_size = 128;  // Set max log file size  
    FLAGS_stop_logging_if_full_disk = true;  // If disk is full  
    google::InstallFailureWriter(&glog_CrashWriter);
    google::InstallFailureSignalHandler();
}

//
// Global variable for shutdownCommunicator
//
Ice::CommunicatorPtr communicator;

//
// Callback for CtrlCHandler
//
void shutdownCommunicator(int sig)
{
    PusherManager::Shutdown();
    communicator->shutdown();
}

int main(int argc, char* argv[])
{
#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL();
    Ice::registerIceUDP();
    Ice::registerIceWS();
#endif

    av_register_all();
    avformat_network_init();

    int status = 0;

    try
    {
        //
        // The communicator initialization removes all Ice-related arguments from argc/argv
        //
        if (argc > 1)
        {
            LOG(FATAL) << "too many arguments";
            return -1;
        }

        //
        // configure google logger
        //
        glog_Initialize();

        std::string err;
        if (!PusherManager::Startup(err))
        {
            LOG(FATAL) << err;
            return -1;
        }

        //
        // CtrlCHandler must be created before the communicator or any other threads are started
        //
        Ice::CtrlCHandler ctrlCHandler;

        //
        // CommunicatorHolder's ctor initializes an Ice communicator,
        // and its dtor destroys this communicator.
        //
        Ice::CommunicatorHolder ich(argc, argv, "conf/config.server");
        communicator = ich.communicator();

        ctrlCHandler.setCallback(shutdownCommunicator);

        try
        {
            Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("Stream");
            stream::OperationPtr operation = new OperationService();
            adapter->add(operation, Ice::stringToIdentity("operation"));
            adapter->activate();

            LOG(INFO) << "Stream Service started";

            communicator->waitForShutdown();
        }
        catch (Ice::Exception &ex)
        {
            LOG(FATAL) << ex.what();
            status = -1;
        }

        PusherManager::Shutdown();
    }
    catch (const std::exception& ex)
    {
        LOG(FATAL) << ex.what();
        status = -1;
    }

    LOG(INFO) << "Stream Service stopped";

    return status;
}


