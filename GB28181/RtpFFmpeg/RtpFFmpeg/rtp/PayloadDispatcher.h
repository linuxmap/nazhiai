
#ifndef _PAYLOADDISPATCHER_HEADER_H_
#define _PAYLOADDISPATCHER_HEADER_H_

#include "Composer.h"

class PayloadDispatcher
{
private:
    struct RTPHeader
    {
        uint8_t version : 2;
        uint8_t padding : 1;
        uint8_t extension : 1;
        uint8_t csrccount : 4;

        uint8_t marker : 1;
        uint8_t payloadtype : 7;

        uint16_t sequencenumber;
        uint32_t timestamp;
        uint32_t ssrc;
    };

public:
    PayloadDispatcher();
    ~PayloadDispatcher();

    void Dispatch(unsigned char* data, size_t size, const std::string& source);

private:
    void Run();

    unsigned char* Parse(RTPHeader* header, unsigned char* data, size_t size, size_t& newsize);

private:
    struct PayLoadHandle
    {
        Composer* composer;
        std::string source;
    };

    std::vector<PayLoadHandle> _payloadhandles;

    bool _dispathing;
    std::thread _dispath;
    std::mutex _locker;
    std::condition_variable _condition;

    struct Packet
    {
        unsigned char* data;
        size_t size;
        std::string source;
    };
    std::queue<Packet> _packets;

private:
    PayloadDispatcher(const PayloadDispatcher&);
    PayloadDispatcher& operator=(const PayloadDispatcher&);
};

#endif

