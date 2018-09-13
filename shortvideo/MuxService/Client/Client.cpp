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
#include "cJSON.h"

using namespace mux;

static void glog_CrashWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

static void glog_Initialize()
{
    google::InitGoogleLogging("Mux-client");
    google::SetStderrLogging(google::GLOG_FATAL);
    FLAGS_log_dir = "logs";
    FLAGS_colorlogtostderr = true;  // Set log color  
    FLAGS_logbufsecs = 0;  // Set log output speed(s)  
    FLAGS_max_log_size = 128;  // Set max log file size  
    FLAGS_stop_logging_if_full_disk = true;  // If disk is full  
    google::InstallFailureWriter(&glog_CrashWriter);
    google::InstallFailureSignalHandler();
}

#include <chrono>
static long long Now()
{
    typedef std::chrono::milliseconds time_precision;
    typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
    now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
    return tp.time_since_epoch().count();
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
    std::string operation;
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

            //
            // CommunicatorHolder's ctor initializes an Ice communicator,
            // and its dtor destroys this communicator.
            //
            Ice::CommunicatorHolder ich(argc, argv, "conf/config.client");

            OperationPrx proxy = OperationPrx::checkedCast(
                ich.communicator()->propertyToProxy("Mux.Proxy")->ice_twoway()->ice_secure(false));
            if (!proxy)
            {
                desc = "invalid proxy";
                status = -1;
                break;
            }

            ParamPtr param(new Param());
            FILE* cfg = fopen("task.json", "r");
            if (cfg)
            {
                fseek(cfg, 0, SEEK_END);
                long size = ftell(cfg);
                fseek(cfg, 0, SEEK_SET);
                char* content = (char*)malloc(size + 1);
                fread(content, size, 1, cfg);
                content[size] = 0;
                fclose(cfg);

                std::string err;
                cJSON *jroot = cJSON_Parse(content);
                if (jroot)
                {
                    do
                    {
                        cJSON* joperation = cJSON_GetObjectItem(jroot, "operation");
                        if (joperation && joperation->type == cJSON_String && strlen(joperation->valuestring) > 0)
                        {
                            operation = joperation->valuestring;
                        }
                        else
                        {
                            err = "operation configuration not found or invalid";
                            break;
                        }

                        cJSON* url = cJSON_GetObjectItem(jroot, "source");
                        if (url && url->type == cJSON_String && strlen(url->valuestring) > 0)
                        {
                            param->url = url->valuestring;
                        }
                        else
                        {
                            err = "source configuration not found or invalid";
                            break;
                        }
                    } while (false);

                    cJSON_Delete(jroot);
                }
                else
                {
                    err = cJSON_GetErrorPtr();
                }

                free(content);
                content = NULL;

                if (!err.empty())
                {
                    throw std::exception(err.c_str());
                }
            }
            else
            {
                throw std::exception("Open configuration file failed: conf/clean.json");
            }
            
            if ("start" == operation)
            {
                std::string result;
                if (!proxy->Start(param, result))
                {
                    desc = result;
                }
            } 
            else
            {
                proxy->Stop(param);
            }

        } while (false);
    }
    catch (const std::exception& ex)
    {
        desc = ex.what();
        status = -1;
    }

    printf("submit task(%s): %s\n", operation.c_str(), desc.c_str());

    return status;
}

