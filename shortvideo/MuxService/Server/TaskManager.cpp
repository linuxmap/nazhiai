#include "TaskManager.h"

#include "cJSON.h"

TaskManager::FileInfo TaskManager::fs;
TaskManager::DbInfo TaskManager::db;

std::mutex TaskManager::_locker;
std::map<std::string, RtspStreamMuxTask*> TaskManager::_tasks;

bool TaskManager::Startup(std::string& err)
{
    FILE* cfg = fopen("conf/mux.json", "r");
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
                cJSON* fsinfo = cJSON_GetObjectItem(jroot, "fs");
                if (fsinfo && fsinfo->type == cJSON_Object)
                {
                    cJSON* host = cJSON_GetObjectItem(fsinfo, "host");
                    if (host && host->type == cJSON_String && strlen(host->valuestring) > 0)
                    {
                        fs.host = host->valuestring;
                    }
                    else
                    {
                        err = "file system(fs) host configuration not found or invalid";
                        break;
                    }

                    cJSON* root = cJSON_GetObjectItem(fsinfo, "root");
                    if (root && root->type == cJSON_String && strlen(root->valuestring) > 0)
                    {
                        fs.root = root->valuestring;
                    }
                    else
                    {
                        err = "file system(fs) root path configuration not found or invalid";
                        break;
                    }

                    cJSON* web_service = cJSON_GetObjectItem(fsinfo, "web_service");
                    if (web_service && web_service->type == cJSON_True)
                    {
                        fs.web_service = true;
                    }
                    else
                    {
                        fs.web_service = false;
                    }

                    cJSON* web = cJSON_GetObjectItem(fsinfo, "web");
                    if (fs.web_service && web && web->type == cJSON_String && strlen(web->valuestring) > 0)
                    {
                        fs.web = web->valuestring;
                    }
                    else
                    {
                        err = "file system(fs) web configuration not found or invalid";
                        break;
                    }

                    cJSON* max_files = cJSON_GetObjectItem(fsinfo, "max_files");
                    if (max_files && max_files->type == cJSON_Number)
                    {
                        fs.max_files = max_files->valueint;
                    }
                    else
                    {
                        err = "file system(fs) directory depth configuration not found or invalid";
                        break;
                    }

                    cJSON* min_duration = cJSON_GetObjectItem(fsinfo, "min_duration");
                    if (min_duration && min_duration->type == cJSON_Number)
                    {
                        fs.min_duration = min_duration->valueint;
                    }
                    else
                    {
                        err = "file system(fs) minimum duration configuration not found or invalid";
                        break;
                    }

                    cJSON* format = cJSON_GetObjectItem(fsinfo, "format");
                    if (format && format->type == cJSON_String && strlen(format->valuestring) > 0)
                    {
                        fs.format = format->valuestring;
                    }
                    else
                    {
                        err = "file system(fs) format configuration not found or invalid";
                        break;
                    }
                }
                else
                {
                    err = "file system(fs) configuration not found or invalid";
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

                cJSON* rtsp = cJSON_GetObjectItem(jroot, "rtsp");
                if (rtsp && rtsp->type == cJSON_Array)
                {
                    int count = cJSON_GetArraySize(rtsp);
                    for (int idx = 0; idx < count; ++idx)
                    {
                        cJSON *link = cJSON_GetArrayItem(rtsp, idx);
                        if (link && link->type == cJSON_String && strlen(link->valuestring) > 0)
                        {
                            StartTask(link->valuestring, std::string());
                        }
                    }
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
        err = "Open configuration file failed: conf/mux.json";
        return false;
    }
}

bool TaskManager::StartTask(const std::string& url, std::string& result)
{
    if (!url.empty())
    {
        std::lock_guard<std::mutex> lg(_locker);
        if (_tasks.end() == _tasks.find(url))
        {
            RtspStreamMuxTask* task = new RtspStreamMuxTask(url, fs.format, fs.min_duration);
            if (task)
            {
                if (task->StartMux(result))
                {
                    _tasks.insert(std::make_pair(url, task));
                    return true;
                }
                task->StopMux();
                delete task;
                task = nullptr;
            }
            return false;
        }
        result = url + " has been in service";
        return false;
    }
    result = "url is empty";
    return false;
}

void TaskManager::StopTask(const std::string& url)
{
    if (!url.empty())
    {
        RtspStreamMuxTask* task = nullptr;
        {
            std::lock_guard<std::mutex> lg(_locker);
            std::map<std::string, RtspStreamMuxTask *>::iterator itr = _tasks.find(url);
            if (_tasks.end() != itr)
            {
                task = itr->second;
                _tasks.erase(itr);
            }
        }
        
        if (task)
        {
            task->StopMux();
            delete task;
            task = nullptr;
        }
    }
}

void TaskManager::Shutdown()
{
    std::lock_guard<std::mutex> lg(_locker);
    std::map<std::string, RtspStreamMuxTask *>::iterator itr = _tasks.begin();
    while (itr != _tasks.end())
    {
        RtspStreamMuxTask* task = itr->second;
        if (task)
        {
            task->StopMux();
            delete task;
            task = nullptr;
        }
        itr = _tasks.erase(itr);
    }
}

