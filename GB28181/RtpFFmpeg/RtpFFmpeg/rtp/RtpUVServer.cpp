#include "RtpUVServer.h"

void RtpUVServer::GuessLocalAddress(char* ip, size_t iplen)
{
    // looking for default ip address
    uv_interface_address_t *info;
    int count = 0;

    uv_interface_addresses(&info, &count);

    for (int i = 0; i < count; ++i)
    {
        uv_interface_address_t& netface = info[i];

        if (netface.address.address4.sin_family == AF_INET)
        {
            uv_ip4_name(&netface.address.address4, ip, iplen);
            break; // get ip address of the first net interface 
        }
        else if (netface.address.address4.sin_family == AF_INET6)
        {
            uv_ip6_name(&netface.address.address6, ip, iplen);
        }
    }

    uv_free_interface_addresses(info, count);
}

RtpUVServer::RtpUVServer()
    : _loop()
    , _send_socket(), _recv_socket()
    , _dispatcher()
{
    uv_loop_init(&_loop);
}

RtpUVServer::~RtpUVServer()
{
    uv_loop_close(&_loop);
}

void RtpUVServer::FeedPacket(unsigned char* data, size_t size, const std::string& source)
{
    _dispatcher.Dispatch(data, size, source);
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    *buf = uv_buf_init((char*)malloc(suggested_size), (int)suggested_size);
}

void on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned)
{
    RtpUVServer* server = (RtpUVServer*)(req->data);
    if (nread < 0) 
    {
        uv_close((uv_handle_t*)req, nullptr);
    }
    else
    {
        // construct source information
        char source[32] = { 0 };
        sockaddr_in* addrin = (sockaddr_in*)addr;
        uv_ip4_name(addrin, source, 32);

        char port[16] = { 0 };
        sprintf_s(port, ":%d", ntohs(addrin->sin_port));
        strcat(source, port);

        // feed packet
        server->FeedPacket((unsigned char*)buf->base, nread, source);
    }
}

int RtpUVServer::Run(int port, const std::string& ip)
{
    int res = 0;
    do 
    {
        res = uv_udp_init(&_loop, &_recv_socket);
        if (res < 0)
        {
            break;
        }

        char addr[256] = {0};
        if (ip.empty())
        {
            GuessLocalAddress(addr, sizeof(addr));
        } 
        else
        {
            strncpy(addr, ip.c_str(), ip.size());
        }

        struct sockaddr_in recv_addr;
        res = uv_ip4_addr(addr, port, &recv_addr);
        if (res < 0)
        {
            break;
        }

        res = uv_udp_bind(&_recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
        if (res < 0)
        {
            break;
        }

        res = uv_udp_recv_start(&_recv_socket, alloc_buffer, on_read);
    } while (false);

    if (res >= 0)
    {
        _recv_socket.data = this;
        return uv_run(&_loop, UV_RUN_DEFAULT);
    } 
    return res;
}

void RtpUVServer::Exit()
{
    uv_stop(&_loop);
}

