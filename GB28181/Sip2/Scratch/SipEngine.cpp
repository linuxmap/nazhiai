#include "SipEngine.h"

#include "tinyxml2.h"

SipEngine::SipEngine(const SipParam& sipParam)
    : _running(false), _engine()
    , _sipParam(sipParam)
    , _ctx(nullptr)
    , _serverSipUsers()
    , _rtpPorts()
{
    for (unsigned short mport = _sipParam.rtp_minport; mport <= _sipParam.rtp_maxport; ++mport)
    {
        for (unsigned short i = 0; i < _sipParam.rtp_max_cli; ++i)
        {
            _rtpPorts.push(mport);
        }
    }
}

SipEngine::~SipEngine()
{
}

bool SipEngine::init(std::string& err)
{
    std::string transtr = "UDP";
    int transport = IPPROTO_UDP;
    if (_sipParam.tcp)
    {
        transport = IPPROTO_TCP;
        transtr = "TCP";
    } 

    if (_sipParam.secure)
    {
        transtr = "TLS";
    }
    
    if (eXosip_listen_addr(_ctx, transport, _sipParam.ip.c_str(), _sipParam.port, AF_INET, _sipParam.secure) != 0)
    {
        eXosip_quit(_ctx);
        err = "set sip socket information(" + _sipParam.ip + ":" + std::to_string(_sipParam.port) + ") over " + transtr + " failed";
        return false;
    }

    _running = true;
    _engine = std::thread(&SipEngine::Engine, this);

    err = "startup sip success (" + _sipParam.ip + ":" + std::to_string(_sipParam.port) + ") over " + transtr;

    return true;
}

void SipEngine::uninit()
{
    _running = false;
    if (_ctx)
    {
        eXosip_quit(_ctx);
        _ctx = nullptr;
    }
    if (_engine.joinable())
    {
        _engine.join();
    }
}

bool SipEngine::ParseMessageContent(char* content, long long len, StreamIds& streamIds)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(content, len) == 0)
    {
        tinyxml2::XMLElement* root = doc.RootElement();
        const char* name = root->Name();
        if (root && name && osip_strcasecmp(name, "Response") == 0) // <Response>
        {
            const char* text = nullptr;
            tinyxml2::XMLNode* node1 = root->FirstChild();
            while (node1)
            {
                tinyxml2::XMLElement* el1 = node1->ToElement();
                name = el1->Name();
                if (name)
                {
                    text = el1->GetText();

                    // not stream list response
                    if (osip_strcasecmp(name, "CmdType") == 0 && (!text || osip_strcasecmp(text, "Catalog"))) // <CmdType>
                    {
                        break;
                    }

                    // stream list xml
                    if (osip_strcasecmp(name, "DeviceList") == 0) // <DeviceList>
                    {
                        tinyxml2::XMLNode* node2 = el1->FirstChild();
                        while (node2) // <Item>
                        {
                            tinyxml2::XMLElement* el2 = node2->ToElement();
                            name = el2->Name();

                            if (name && osip_strcasecmp(name, "Item") == 0)
                            {
                                tinyxml2::XMLNode* node3 = el2->FirstChild();
                                while (node3) // <Item><...>
                                {
                                    tinyxml2::XMLElement* el3 = node3->ToElement();
                                    name = el3->Name();
                                    text = el3->GetText();
                                    if (name && osip_strcasecmp(name, "DeviceID") == 0 && text)
                                    {
                                        streamIds.push_back(text);
                                        break;
                                    }
                                    node3 = node2->NextSibling();
                                }
                            }
                            node2 = node2->NextSibling();
                        }
                        break;
                    }
                }
                node1 = node1->NextSibling();
            }
        }
    }
    return streamIds.size() > 0;
}

bool SipEngine::FetchUsableRtport(unsigned short& rtport)
{
    if (_rtpPorts.size() > 0)
    {
        rtport = _rtpPorts.front();
        _rtpPorts.pop();
        return true;
    }
    return false;
}

void SipEngine::ReturnUsableRtport(unsigned short rtport)
{
    _rtpPorts.push(rtport);
}

void SipEngine::SendAck(eXosip_event_t* evt)
{
    osip_message_t *ack = NULL;
    if (eXosip_call_build_ack(_ctx, evt->did, &ack) == 0)
    {
        eXosip_lock(_ctx);
        eXosip_call_send_ack(_ctx, evt->did, ack);
        eXosip_unlock(_ctx);
    }
}

void SipEngine::SendAnswer(eXosip_event_t* evt)
{
    osip_message_t *answer = NULL;
    if (eXosip_message_build_answer(_ctx, evt->tid, 200, &answer) == 0)
    {
        eXosip_lock(_ctx);
        eXosip_message_send_answer(_ctx, evt->tid, 200, answer);
        eXosip_unlock(_ctx);
    }
}

void SipEngine::SendInvite(const std::string& serverId, StreamIds& streamIds)
{
    ServerSipUsers::iterator itr = _serverSipUsers.find(serverId);
    if (itr != _serverSipUsers.end())
    {
        SipUser& sipUser = itr->second;
        unsigned short rtport = 0;
        
        if (SipPlay != sipUser.state && FetchUsableRtport(rtport))
        {
            for each (StreamId streamId in streamIds)
            {
                char str[1024] = { 0 };
                _snprintf(str, 1024,
                    "v=0\n"
                    "o=%s 0 0 IN IP4 %s\n"
                    "s=Play\n"
                    "c=IN IP4 %s\n"
                    "t=0 0\n"
                    "m=video %d RTP/AVP 96 98 97\n"
                    "a=recvonly\n"
                    "a=rtpmap:96 PS/90000\n"
                    "a=rtpmap:97 MPEG4/90000\n"
                    "a=rtpmap:98 H264/90000\n"
                    "y=0100000001\n",
                    streamId.c_str(),
                    _sipParam.rtpip.c_str(),
                    _sipParam.rtpip.c_str(),
                    rtport
                    );

                osip_message_t *invite = nullptr;
                if (0 == eXosip_call_build_initial_invite(_ctx, &invite, sipUser.to.c_str(), sipUser.from.c_str(), nullptr, nullptr))
                {
                    osip_message_set_subject(invite, "34020000001110000001:0"); // this should be changed according to gb28181
                    if (osip_message_set_body(invite, str, strlen(str)) == 0
                        && osip_message_set_content_type(invite, "application/sdp") == 0)
                    {
                        eXosip_lock(_ctx);
                        if (0 == eXosip_call_send_initial_invite(_ctx, invite))
                        {
                            sipUser.state = SipPlay;
                            sipUser.rtpport = rtport;
                        }
                        eXosip_unlock(_ctx);
#ifdef _DEBUG
                        printf("%s:%s --> SipPlay\n", sipUser.sip_username.c_str(), streamId.c_str());
#endif
                    }
                }
            }
        }
    }
}

bool SipEngine::Startup(std::string& err)
{
    if (!_running)
    {
        TRACE_INITIALIZE((osip_trace_level_t)_sipParam.log_level, stdout);

        _ctx = eXosip_malloc();;
        if (!_ctx)
        {
            err = "allocate sip context failed";
            return false;
        }

        int result = eXosip_init(_ctx);
        if (result != 0)
        {
            err = "initialize sip context failed";
            return false;
        }

        if (_sipParam.ip.empty())
        {
            char localip[128];
            eXosip_guess_localip(_ctx, AF_INET, localip, 128);

            _sipParam.ip = localip;
        }

        if (_sipParam.ip.empty())
        {
            err = "get local address failed";
            return false;
        }

        if (_sipParam.rtpip.empty())
        {
            _sipParam.rtpip = _sipParam.ip;
        }

        eXosip_set_user_agent(_ctx, _sipParam.agent.c_str());

        return init(err);
    }
    else
    {
        err = "sip context has already startup";
    }
    return false;
}

bool SipEngine::Restart(std::string& err)
{
    if (_running)
    {
        uninit();
        return init(err);
    }
    else
    {
        err = "sip context is not startup yet";
    }
    return false;
}

void SipEngine::Shutdown()
{
    if (_running)
    {
        uninit();
    }
}

void print_sip_info(eXosip_event_t* evt)
{
    if (evt->request)
    {
        printf("======request type:%d, method:%s [cseq:%s %s]======\n", evt->type, evt->request->sip_method, evt->request->cseq->number, evt->request->cseq->method);
    }
    if (evt->ack)
    {
        printf("======ack type:%d, method:%s [cseq:%s %s]======\n", evt->type, evt->ack->sip_method, evt->ack->cseq->number, evt->ack->cseq->method);
    }
    if (evt->response)
    {
        printf("======resp type:%d, method:%s [cseq:%s %s]======\n", evt->type, evt->response->sip_method, evt->response->cseq->number, evt->response->cseq->method);
    }
}

void SipEngine::Engine()
{
    while (_running)
    {
        eXosip_event_t* evt = eXosip_event_wait(_ctx, 0, 50);
        if (!_running || !_ctx)
        {
            if (evt)
            {
                eXosip_event_free(evt);
                evt = nullptr;
            }
            break;
        }
        
        eXosip_lock(_ctx);
        eXosip_automatic_action(_ctx);
        eXosip_unlock(_ctx);

        if (!evt)
        {
            continue;
        }

#ifdef _DEBUG
        print_sip_info(evt);
#endif

        if (evt->type == EXOSIP_MESSAGE_NEW)
        {
            if (osip_strcasecmp(evt->request->sip_method, "REGISTER") == 0)
            {
                osip_uri_t *url = evt->request->from->url;
                ServerId serverId = std::string(url->host) + "@" + std::string(url->username);

                // user not exists
                ServerSipUsers::iterator itr = _serverSipUsers.find(serverId);
                if (itr == _serverSipUsers.end())
                {
                    // usable port
                    if (_rtpPorts.size() > 0)
                    {
                        // get ip and port
                        osip_from_t *from = (osip_from_t*)(evt->request->contacts.node->element);

                        SipUser sipUser;
                        sipUser.ip = from->url->host;
                        sipUser.port = from->url->port;
                        sipUser.sip_username = url->username;

                        // build 401 answer message
                        osip_message_t *answer = NULL;
                        if (0 == eXosip_message_build_answer(_ctx, evt->tid, 401, &answer))
                        {
                            if (osip_message_set_header(answer, "WWW-Authenticate", "Digest realm=\"www.nazhiai.com\",nonce=\"23fa2f4\",algorithm=\"MD5\"") == 0)
                            {
                                eXosip_lock(_ctx);
                                if (0==eXosip_message_send_answer(_ctx, evt->tid, 401, answer))
                                {
                                    sipUser.state = SipUnauthorized;
                                    _serverSipUsers.insert(std::make_pair(serverId, sipUser));
                                }
                                eXosip_unlock(_ctx);
                            }
                        }
                        
#ifdef _DEBUG
                        if (SipUnauthorized == sipUser.state)
                        {
                            printf("%s --> Response Unauthorized\n", sipUser.sip_username.c_str());
                        }
                        else
                        {
                            printf("%s --> Response Unauthorized failed\n", sipUser.sip_username.c_str());
                        }
#endif
                    }
                }
                else
                {
                    SipUser& sipUser = itr->second;
                    if (SipUnauthorized == sipUser.state)
                    {
                        osip_message_t *answer = NULL;
                        if (eXosip_message_build_answer(_ctx, evt->tid, 200, &answer) == 0)
                        {
                            eXosip_lock(_ctx);
                            if (0 == eXosip_message_send_answer(_ctx, evt->tid, 200, answer))
                            {
                                sipUser.state = SipAuthorized;
                            }
                            eXosip_unlock(_ctx);

                            if (SipAuthorized == sipUser.state)
                            {
#ifdef _DEBUG
                                printf("%s --> Response Authorized\n", sipUser.sip_username.c_str());
#endif
                                char to[512] = { 0 };
                                _snprintf(to, 511, "sip:%s@%s:%s",
                                    evt->request->from->url->username, sipUser.ip.c_str(), sipUser.port.c_str());
                                sipUser.to = to;

                                char from[512] = { 0 };
                                _snprintf(from, 511, "sip:%s@%s:%d",
                                    evt->request->to->url->username, _sipParam.ip.c_str(), _sipParam.port);
                                sipUser.from = from;

                                osip_message_t *catalog;
                                if (eXosip_message_build_request(_ctx, &catalog, "MESSAGE", to, from, NULL) == 0)
                                {
                                    char tmp[4096];
                                    _snprintf(tmp, 4096,
                                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                        "<Query>\n"
                                        "<CmdType>Catalog</CmdType>\n"
                                        "<SN>1</SN>\n"
                                        "<DeviceID>%s</DeviceID>\n"
                                        "</Query>\n", sipUser.sip_username.c_str());

                                    if (osip_message_set_body(catalog, tmp, strlen(tmp)) == 0
                                        && osip_message_set_content_type(catalog, "Application/MANSCDP+xml") == 0)
                                    {
                                        eXosip_lock(_ctx);
                                        int reqcatalog = eXosip_message_send_request(_ctx, catalog);
                                        eXosip_unlock(_ctx);

#ifdef _DEBUG
                                        if (reqcatalog)
                                        {
                                            printf("%s --> Request Catalog(%s --> %s:%d)\n", sipUser.sip_username.c_str(), from, to, reqcatalog);
                                        } 
                                        else
                                        {
                                            printf("%s --> Request Catalog failed(%s --> %s:%d)\n", sipUser.sip_username.c_str(), from, to, reqcatalog);
                                        }
#endif
                                    }
                                }
                            }
#ifdef _DEBUG
                            else
                            {
                                printf("%s --> Response Authorized failed\n", sipUser.sip_username.c_str());
                            }
#endif
                        }
                        
                        if (sipUser.state != SipAuthorized)
                        {
                            _serverSipUsers.erase(itr);
                        }
                    }
                    else
                    {
                        SendAnswer(evt);
                    }
                }
            }
            else if (osip_strcasecmp(evt->request->sip_method, "MESSAGE") == 0)
            {
                SendAnswer(evt);

                osip_message_t *request = evt->request;
                if (request->content_type
                    && osip_strcasecmp(request->content_type->type, "Application") == 0
                    && osip_strcasecmp(request->content_type->subtype, "MANSCDP+xml") == 0)
                {
                    osip_body_t* body = nullptr;
                    if (request->bodies.nb_elt > 0)
                    {
                        StreamIds streamIds;
                        body = (osip_body_t*)request->bodies.node->element;
                        if (ParseMessageContent(body->body, body->length, streamIds))
                        {
                            osip_uri_t *url = evt->request->from->url;
                            SendInvite(std::string(url->host) + "@" + std::string(url->username), streamIds);
                        }
                    }
                }
            }
            else
            {
                SendAnswer(evt);
            }
        }
        else if (EXOSIP_CALL_ANSWERED == evt->type
            || EXOSIP_MESSAGE_ANSWERED == evt->type)
        {            
            SendAck(evt);
        }
        else if (EXOSIP_CALL_CLOSED == evt->type)
        {
            SendAnswer(evt);

            osip_uri_t *url = evt->request->to->url;
            ServerId serverId = std::string(url->host) + "@" + std::string(url->username);

            // user exists
            ServerSipUsers::iterator itr = _serverSipUsers.find(serverId);
            if (itr != _serverSipUsers.end())
            {
#ifdef _DEBUG
                printf("%s --> SipStop\n", itr->second.sip_username.c_str());
#endif
                ReturnUsableRtport(itr->second.rtpport);
                _serverSipUsers.erase(itr);
            }
        }
        else
        {
            // do something
        }
        eXosip_event_free(evt);
    }
}
