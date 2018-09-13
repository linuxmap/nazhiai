#ifndef _OPENCVCAPTURE_HEADER_H_
#define _OPENCVCAPTURE_HEADER_H_

#include "BaseDecoder.h"

class OpencvDecoder : public BaseDecoder
{
public:
    OpencvDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~OpencvDecoder();

    void Create() throw(BaseException);
    void Destroy() throw(BaseException);

    inline const char* Name() const
    {
        return "OpencvDecoder_";
    }

protected:
    bool ReadFrame(cv::Mat& frame) override;
    bool ReadFrame(cv::cuda::GpuMat& frame) override;

private:
    cv::VideoCapture _video;

private:
    OpencvDecoder();
    OpencvDecoder(const OpencvDecoder&);
    OpencvDecoder& operator=(const OpencvDecoder&);
};

#endif

