
#ifndef _COMPOSER_HEADER_H_
#define _COMPOSER_HEADER_H_

#include "../ffmpeg/FFmpeger.h"

class Composer
{
public:
    Composer(unsigned char payloadtype, size_t size, const std::string& source);
    ~Composer();

    void FeedPacket(unsigned char* packet, size_t size, unsigned short seq, unsigned char marker);

protected:
    size_t _size;
    unsigned char* _buffer;

    size_t _real;
    unsigned short _preseq;

    FFmpeger* _decoder;

private:
    Composer();
    Composer(const Composer&);
    Composer& operator=(const Composer&);
};

#endif

