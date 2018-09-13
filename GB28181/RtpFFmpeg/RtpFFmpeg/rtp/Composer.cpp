#include "Composer.h"

#include <stdlib.h>
#include <memory.h>

Composer::Composer(unsigned char payloadtype, size_t size, const std::string& source)
    : _size(size), _buffer((unsigned char*)malloc(_size))
    , _real(0), _preseq(0)
    , _decoder(FFmpeger::CreateFFmpeger(payloadtype, source))
{
}

Composer::~Composer()
{
    if (_buffer)
    {
        free(_buffer);
        _buffer = nullptr;
    }

    FFmpeger::DestroyFFmpeger(_decoder);
}

void Composer::FeedPacket(unsigned char* packet, size_t size, unsigned short, unsigned char marker)
{
    size_t real = _real + size;
    if (real <= _size && size > 0)
    {
        memcpy(_buffer + _real, packet, size);
        _real = real;
    }

    if (marker)
    {
        _decoder->FeedFrame(_buffer, _real);
        _real = 0;
    }
}


