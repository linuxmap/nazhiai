#ifndef _DIRECTORYDECODER_HEADER_H_
#define _DIRECTORYDECODER_HEADER_H_

#include "BaseDecoder.h"
#include "FileSystem.h"
#include <queue>
#include "jpeg_codec.h"

class DirectoryDecoder : public BaseDecoder
{
public:
    DirectoryDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~DirectoryDecoder();

    const char* Name() const
    {
        return "DirectoryDecoder";
    }

protected:
    bool Init();
    void Uninit();
    bool ReadFrame(DecodedFrame& frame) override;

private:
    unsigned long long _currentFramePostion;
    FileSystem::FNodeVector _files;

    jpeg_codec::decoder_instance* _decoder;
    unsigned char* _read_buffer;

    cv::Mat _frame;
};

#endif

