
#include "Configuration.h"

#include "cJSON.h"

#include "Maria.h"

Configuration::ActionInfo Configuration::action;
Configuration::DbInfo Configuration::db;

bool Configuration::Load(std::string& err)
{
    FILE* cfg = fopen("conf/clean.json", "r");
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
                cJSON* act = cJSON_GetObjectItem(jroot, "action");
                if (act && act->type == cJSON_Object)
                {
                    cJSON* hostid = cJSON_GetObjectItem(act, "hostid");
                    if (hostid && hostid->type == cJSON_Number)
                    {
                        action.hostid = hostid->valueint;
                    }
                    else
                    {
                        err = "action hostid configuration not found or invalid";
                        break;
                    }

                    cJSON* frequency = cJSON_GetObjectItem(act, "frequency");
                    if (frequency && frequency->type == cJSON_Number)
                    {
                        action.frequency = frequency->valueint;
                    }
                    else
                    {
                        err = "action frequency configuration not found or invalid";
                        break;
                    }

                    cJSON* margin = cJSON_GetObjectItem(act, "margin");
                    if (margin && margin->type == cJSON_Number)
                    {
                        action.margin = margin->valueint;
                    }
                    else
                    {
                        err = "action margin configuration not found or invalid";
                        break;
                    }
                }
                else
                {
                    err = "action configuration not found or invalid";
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
                        err = "database(db) local configuration not found or invalid";
                        break;
                    }
                }
                else
                {
                    err = "database(db) configuration not found or invalid";
                }

                Maria maria(db.url);
                if (!maria.OneHost(action.hostid, action.host, action.root))
                {
                    err = "short video host information not found";
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
        err = "Open configuration file failed: conf/clean.json";
        return false;
    }
}

void Configuration::Unload()
{
}

