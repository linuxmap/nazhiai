#include "PayloadDispatcher.h"

PayloadDispatcher::PayloadDispatcher()
    : _payloadhandles()
    , _dispathing(false), _dispath(), _locker(), _condition()
    , _packets()
{
    _dispathing = true;
    _dispath = std::thread(&PayloadDispatcher::Run, this);
}

PayloadDispatcher::~PayloadDispatcher()
{
    _dispathing = false;
    _condition.notify_one();
    if (_dispath.joinable())
    {
        _dispath.join();
    }
}

void PayloadDispatcher::Dispatch(unsigned char* data, size_t size, const std::string& source)
{
    Packet packet{ data, size, source };
    std::lock_guard<std::mutex> lg(_locker);
    _packets.push(packet);
    _condition.notify_one();
}

void PayloadDispatcher::Run()
{
    while (_dispathing)
    {
        Packet packet{nullptr, 0, ""};
        {
            std::unique_lock<std::mutex> ul(_locker);
            _condition.wait_for(ul, std::chrono::milliseconds(500), [this]() { return !_packets.empty() || !_dispathing; });

            if (!_dispathing || _packets.empty())
            {
                continue;
            }

            packet = _packets.front();
            _packets.pop();
        }

        RTPHeader rtp;
        size_t payload_len = 0;
        unsigned char* payload = Parse(&rtp, packet.data, packet.size, payload_len);
        if (payload)
        {
            Composer* composer = nullptr;
            for each (PayLoadHandle handle in _payloadhandles)
            {
                if (handle.source == packet.source)
                {
                    composer = handle.composer;
                    break;
                }
            }

            if (!composer)
            {
                composer = new Composer(rtp.payloadtype, 1024 * 1024, packet.source);
                if (composer)
                {
                    _payloadhandles.push_back(PayLoadHandle{ composer, packet.source });
                }
            }

            if (composer)
            {
                composer->FeedPacket(payload, payload_len, rtp.sequencenumber, rtp.marker);
            }
        }
        else
        {
            printf("invalid rtp packet\n");
        }

        free(packet.data);
    }
}

unsigned char* PayloadDispatcher::Parse(RTPHeader* header, unsigned char* data, size_t size, size_t& newsize)
{
    newsize = size;

    if (size < 12)
    {
        //Too short to be a valid RTP header.  
        return nullptr;
    }

    header->version = data[0] >> 6;
    if (header->version != 2)
    {
        //Currently, the version is 2, if is not 2, unsupported.  
        return nullptr;
    }

    header->version = data[0] & 0x20;
    if (header->padding)
    {
        // Padding present.  
        size_t paddingLength = data[newsize - 1];
        if (paddingLength + 12 > newsize)
        {
            return nullptr;
        }
        newsize -= paddingLength;
    }

    header->marker = data[1] >> 7;
    header->payloadtype = data[1] & 0x7F;

    header->csrccount = data[0] & 0x0f;
    size_t payloadOffset = 12 + 4 * header->csrccount;
    if (newsize < payloadOffset)
    {
        // Not enough data to fit the basic header and all the CSRC entries.  
        return nullptr;
    }

    header->extension = data[0] & 0x10;
    if (header->extension)
    {
        // Header extension present.  
        if (newsize < payloadOffset + 4)
        {
            // Not enough data to fit the basic header, all CSRC entries and the first 4 bytes of the extension header.  
            return nullptr;
        }

        const uint8_t *extensionData = (const uint8_t *)&data[payloadOffset];
        size_t extensionLength = 4 * (extensionData[2] << 8 | extensionData[3]);
        if (newsize < payloadOffset + 4 + extensionLength)
        {
            return nullptr;
        }
        payloadOffset += (4 + extensionLength);
    }

    header->timestamp = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
    header->ssrc = data[8] << 24 | data[9] << 16 | data[10] << 8 | data[11];
    header->sequencenumber = data[2] << 8 | data[3];

    newsize -= payloadOffset;

    return data + payloadOffset;
}



