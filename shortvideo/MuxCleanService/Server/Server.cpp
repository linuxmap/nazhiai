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

#include "CleanShortVideoService.h"

#include "Configuration.h"

#include "ProxyManager.h"

#include "CleanTaskManager.h"

static void glog_CrashWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

static void glog_Initialize()
{
    google::InitGoogleLogging("MuxCleanService");
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
CleanTaskManager* manager = nullptr;
void shutdownCommunicator(int sig)
{
    if (manager)
    {
        manager->Destroy();
        delete manager;
        manager = nullptr;
    }
    communicator->shutdown();
}

int main(int argc, char* argv[])
{
#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL();
    Ice::registerIceUDP();
    Ice::registerIceWS();
#endif

    int status = 0;
    std::string desc("success");
    try
    {
        do 
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
                throw std::exception("too many arguments");
            }

            std::string err;
            if (!Configuration::Load(err))
            {
                throw std::exception(err.c_str());
            }

            if (!ProxyManager::Initialize(argc, argv, err))
            {
                throw std::exception(err.c_str());
            }

            manager = new CleanTaskManager(Configuration::action.hostid);
            if (!manager)
            {
                throw std::exception("start clean task manager failed");
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
                Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("MuxCleanService");
                clean::CleanShortVideoPtr service = new CleanShortVideoService();
                adapter->add(service, Ice::stringToIdentity("clean"));
                adapter->activate();

                manager->Startup();

                LOG(INFO) << "Mux Clean Service started success";

                communicator->waitForShutdown();
            }
            catch (Ice::Exception &ex)
            {
                throw std::exception(ex);
            }
        } while (false);
    }
    catch (const std::exception& ex)
    {
        desc = ex.what();
        status = -1;
    }
    
    Configuration::Unload();
    if (manager)
    {
        manager->Destroy();
        delete manager;
        manager = nullptr;
    }

    LOG(INFO) << "Mux Clean Service stopped: " << desc;

    return status;
}


