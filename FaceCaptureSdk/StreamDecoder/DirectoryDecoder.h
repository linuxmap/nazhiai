#ifndef _DIRECTORYDECODER_HEADER_H_
#define _DIRECTORYDECODER_HEADER_H_

#include "BaseDecoder.h"
#include "FileSystem.h"
#include <queue>

class DirectoryDecoder : public BaseDecoder
{
public:
    DirectoryDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~DirectoryDecoder();

    void Create() throw(BaseException);
    void Destroy() throw(BaseException);

    inline const char* Name() const
    {
        return "DirectoryDecoder_";
    }

protected:
    bool ReadFrame(cv::Mat& frame) override;
    bool ReadFrame(cv::cuda::GpuMat& frame) override;

private:
    unsigned long long _currentFramePostion;
    FileSystem::FNodeVector _files;
};

#endif

