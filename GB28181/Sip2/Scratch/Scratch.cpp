// Scratch.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "SipEngine.h"

#include <stdio.h>

int _tmain(int argc, _TCHAR* argv[])
{
    SipParam param;
    param.port = 8550;
    param.agent = "NaZhiAI";
    param.rtp_maxport = 8556;
    param.log_level = TRACE_LEVEL1;
    SipEngine eng(param);

    std::string err;
    if (!eng.Startup(err))
    {
        printf("%s\n", err.c_str());
        return -1;
    }
    printf("%s\n", err.c_str());

    Sleep(2 * 60 * 60 * 1000);
    
    eng.Shutdown();

    return 0;
}

