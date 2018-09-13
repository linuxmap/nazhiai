#include "PusherManager.h"

#include "cJSON.h"

PusherManager::MediaInfo PusherManager::md;
PusherManager::DbInfo PusherManager::db;

std::mutex PusherManager::_locker;
std::queue<RtmpPusher*> PusherManager::_free_pushers;

bool PusherManager::Startup(std::string& err)
{
    FILE* cfg = fopen("conf/stream.json", "r");
    if (cfg)
    {
        fseek(cfg, 0, SEEK_END);
        long size = ftell(cfg);
        fseek(cfg, 0, SEEK_SET);
        char* content = (char*)malloc(size + 1);
        fread(content, size, 1, cfg);
        content[size] = 0;
        fclose(cfg);

        cJSON *jroot = cJSON_Parse(content);
        if (jroot)
        {
            do 
            {
                cJSON* media = cJSON_GetObjectItem(jroot, "media");
                if (media && media->type == cJSON_Object)
                {
                    cJSON* root = cJSON_GetObjectItem(media, "root");
                    if (root && root->type == cJSON_String && strlen(root->valuestring) > 0)
                    {
                        md.root = root->valuestring;
                    }
                    else
                    {
                        err = "media information root path configuration not found or invalid";
                        break;
                    }

                    cJSON* rtmp = cJSON_GetObjectItem(media, "rtmp");
                    if (rtmp && rtmp->type == cJSON_String && strlen(rtmp->valuestring) > 0)
                    {
                        md.rtmp = rtmp->valuestring;
                    }
                    else
                    {
                        err = "media information rtmp configuration not found or invalid";
                        break;
                    }
                }
                else
                {
                    err = "media information configuration not found or invalid";
                }

                cJSON* dbinfo = cJSON_GetObjectItem(jroot, "db");
                if (dbinfo && dbinfo->type == cJSON_Object)
                {
                    cJSON* url = cJSON_GetObjectItem(dbinfo, "url");
                    if (url && url->type == cJSON_String && strlen(url->valuestring) > 0)
                    {
                        db.url = url->valuestring;
                    }
                    else
                    {
                        err = "database(db) url configuration not found or invalid";
                        break;
                    }

                    cJSON* username = cJSON_GetObjectItem(dbinfo, "username");
                    if (username && username->type == cJSON_String && strlen(username->valuestring) > 0)
                    {
                        db.username = username->valuestring;
                    }
                    else
                    {
                        err = "database(db) username configuration not found or invalid";
                        break;
                    }

                    cJSON* password = cJSON_GetObjectItem(dbinfo, "password");
                    if (password && password->type == cJSON_String && strlen(password->valuestring) > 0)
                    {
                        db.password = password->valuestring;
                    }
                    else
                    {
                        err = "database(db) password configuration not found or invalid";
                        break;
                    }
                }
                else
                {
                    err = "database(db) configuration not found or invalid";
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

        return err.empty();
    }
    else
    {
        err = "Open configuration file failed: conf/stream.json";
        return false;
    }
}

bool PusherManager::PushStream(const std::string& file, std::string& result)
{
    if (!file.empty())
    {
        RtmpPusher* task = Alloc();
        if (task)
        {
            if (!task->Push(md.root + file, result))
            {
                Free(task);
                return false;
            }
            return true;
        }
        
        result ="Stream Service can not allocate new service";
        return false;
    }
    result = "url or file is empty";
    return false;
}

void PusherManager::Shutdown()
{
    std::lock_guard<std::mutex> lg(_locker);
    while (!_free_pushers.empty())
    {
        RtmpPusher* pusher = _free_pushers.front();
        _free_pushers.pop();

        if (pusher)
        {
            delete pusher;
            pusher = nullptr;
        }
    }
}

RtmpPusher* PusherManager::Alloc()
{
    std::lock_guard<std::mutex> lg(_locker);
    if (!_free_pushers.empty())
    {
        RtmpPusher* pusher = _free_pushers.front();
        _free_pushers.pop();
        return pusher;
    }
    else
    {
        return new RtmpPusher(md.rtmp);
    }
}

void PusherManager::Free(RtmpPusher* pusher)
{
    if (pusher)
    {
        std::lock_guard<std::mutex> lg(_locker);
        _free_pushers.push(pusher);
    }
}

