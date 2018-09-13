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

using namespace clean;

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

        clean::ParamPtr param(new clean::Param());
        FILE* cfg = fopen("trigger.json", "r");
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
                    cJSON* hostid = cJSON_GetObjectItem(jroot, "hostid");
                    if (hostid && hostid->type == cJSON_Number)
                    {
                        param->hostid = hostid->valueint;
                    }
                    else
                    {
                        err = "hostid configuration not found or invalid";
                        break;
                    }

                    cJSON* source = cJSON_GetObjectItem(jroot, "source");
                    if (source && source->type == cJSON_String && strlen(source->valuestring) > 0)
                    {
                        param->url = source->valuestring;
                    }
                    else
                    {
                        err = "source configuration not found or invalid";
                        break;
                    }

                    cJSON* begin = cJSON_GetObjectItem(jroot, "begin");
                    if (begin && begin->type == cJSON_String && strlen(begin->valuestring) > 0)
                    {
                        param->begin = begin->valuestring;
                    }
                    else
                    {
                        err = "begin configuration not found or invalid";
                        break;
                    }

                    cJSON* end = cJSON_GetObjectItem(jroot, "end");
                    if (end && end->type == cJSON_String && strlen(end->valuestring) > 0)
                    {
                        param->end = end->valuestring;
                    }
                    else
                    {
                        err = "end configuration not found or invalid";
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

        //
        // CommunicatorHolder's ctor initializes an Ice communicator,
        // and its dtor destroys this communicator.
        //
        Ice::CommunicatorHolder ich(argc, argv, "conf/config.client");

        CleanShortVideoPrx twoway = CleanShortVideoPrx::checkedCast(
            ich.communicator()->propertyToProxy("MuxCleanService.Proxy")->ice_twoway()->ice_secure(false));
        if (!twoway)
        {
            throw std::exception("invalid proxy: MuxClean.Proxy");
        }

        std::string err;
        if (twoway->Clean(param, err))
        {
            printf("submit trigger task success\n");
        }
        else
        {
            printf("%s\n", err.c_str());
        }

        system("pause");
    }
    catch (const std::exception& ex)
    {
        LOG(FATAL) << ex.what();
        status = -1;
    }

    return status;
}

