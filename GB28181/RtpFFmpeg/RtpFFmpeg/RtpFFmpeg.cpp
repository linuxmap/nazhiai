// RtpFFmpeg.cpp : �������̨Ӧ�ó������ڵ㡣
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


