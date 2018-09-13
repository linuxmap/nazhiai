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

#include "TaskManager.h"

#ifdef DUMP_RELEASE

#include <time.h>
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    // 返回EXCEPTION_CONTINUE_SEARCH，让程序停止运行   
    LONG ret = EXCEPTION_CONTINUE_SEARCH;

    HANDLE hFile = ::CreateFile(L"crash.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION ExInfo;

        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = NULL;

        // write the dump
        BOOL bOK = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
        ret = EXCEPTION_EXECUTE_HANDLER;
        ::CloseHandle(hFile);
    }

    return ret;
}

#endif

static void glog_CrashWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

static void glog_Initialize()
{
    google::InitGoogleLogging("MuxService");
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
    TaskManager::Shutdown();
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

#ifdef DUMP_RELEASE
    ::SetUnhandledExceptionFilter(TopLevelFilter);
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
                desc = "too many arguments";
                status = -1;
                break;
            }

            std::string err;
            if (!TaskManager::Startup(err))
            {
                desc = err;
                status = -1;
                break;
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
                Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("Mux");
                mux::OperationPtr operation = new OperationService();
                adapter->add(operation, Ice::stringToIdentity("operation"));
                adapter->activate();

                LOG(INFO) << "Mux Service started";

                communicator->waitForShutdown();
            }
            catch (Ice::Exception &ex)
            {
                desc = ex.what();
                status = -1;
            }

            TaskManager::Shutdown();
        } while (false);
    }
    catch (const std::exception& ex)
    {
        desc = ex.what();
        status = -1;
    }

    LOG(INFO) << "Mux Service stopped: " << desc;

    return status;
}


