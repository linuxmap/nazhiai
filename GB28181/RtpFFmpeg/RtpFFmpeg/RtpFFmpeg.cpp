// RtpFFmpeg.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "rtp/RtpUVServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
    RtpUVServer server;
    server.Run(8554);

    system("pause");
    return 0;
}


