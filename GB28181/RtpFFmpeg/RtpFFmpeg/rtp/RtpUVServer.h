
#ifndef _RTPUVSERVER_HEADER_H_
#define _RTPUVSERVER_HEADER_H_

#include "uv.h"

#include "PayloadDispatcher.h"

#include <string>

class RtpUVServer 
{
public:
    RtpUVServer();
    ~RtpUVServer();

    void FeedPacket(unsigned char* data, size_t size, const std::string& source);

    int Run(int port, const std::string& ip = "");

    void Exit();

private:
    void GuessLocalAddress(char*, size_t);

private:
    uv_loop_t _loop;

    uv_udp_t _send_socket;
    uv_udp_t _recv_socket;

    PayloadDispatcher _dispatcher;

private:
    RtpUVServer(const RtpUVServer&);
    RtpUVServer& operator=(const RtpUVServer&);
};

#endif

