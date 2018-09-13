
#ifndef _SIPENGINE_HEADER_H_
#define _SIPENGINE_HEADER_H_

#include <eXosip2/eXosip.h>

#include <string>
#include <thread>

#include <queue>
#include <map>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

struct SipParam 
{
    bool tcp = false;

    unsigned short port = 0;
    std::string ip = {};

    unsigned short rtp_minport = 8554;
    unsigned short rtp_maxport = 8554;
    unsigned short rtp_max_cli = 10;
    std::string rtpip = {};

    bool secure = false;
    std::string agent = {};

    int log_level = TRACE_LEVEL1;
};

class SipEngine
{
private:
    enum { SipStop = -1, SipUnauthorized, SipAuthorized, SipPlay };
    typedef struct _SipUser
    {
        int state = SipStop;
        std::string ip = {};
        std::string port = {};
        std::string sip_username = {};
        std::string sip_password = {};

        std::string from = {};
        std::string to = {};

        unsigned short rtpport = 0;
    } SipUser;
    typedef std::string ServerId;
    typedef std::map<ServerId, SipUser> ServerSipUsers;

    typedef std::queue<unsigned short> RtpPorts;

    typedef std::string StreamId;
    typedef std::vector<StreamId> StreamIds;

public:
    SipEngine(const SipParam& sipParam);
    ~SipEngine();

    bool Startup(std::string& err);
    bool Restart(std::string& err);
    void Shutdown();

protected:
    void Engine();

private:
    bool init(std::string& err);
    void uninit();
    
    bool ParseMessageContent(char* content, long long len, StreamIds& streamIds);

    bool FetchUsableRtport(unsigned short&);
    void ReturnUsableRtport(unsigned short);

    void SendAck(eXosip_event_t* evt);
    void SendAnswer(eXosip_event_t* evt);

    void SendInvite(const std::string& serverId, StreamIds& streamIds);

private:
    volatile bool _running;
    std::thread _engine;

    SipParam _sipParam;

    struct eXosip_t* _ctx;

private:
    ServerSipUsers _serverSipUsers;

    RtpPorts _rtpPorts;

private:
    SipEngine();
    SipEngine(const SipEngine&);
    SipEngine& operator=(const SipEngine&);
};

#endif

