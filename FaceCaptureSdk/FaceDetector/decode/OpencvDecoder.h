#ifndef _OPENCVCAPTURE_HEADER_H_
#define _OPENCVCAPTURE_HEADER_H_

#include "BaseDecoder.h"

class OpencvDecoder : public BaseDecoder
{
public:
    OpencvDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~OpencvDecoder();

    const char* Name() const
    {
        return "OpencvDecoder";
    }

protected:
    bool Init();
    void Uninit();
    bool ReadFrame(DecodedFrame& frame) override;

private:
    cv::VideoCapture _video;

private:
    OpencvDecoder();
    OpencvDecoder(const OpencvDecoder&);
    OpencvDecoder& operator=(const OpencvDecoder&);
};

#endif

