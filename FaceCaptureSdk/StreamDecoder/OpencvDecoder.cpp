
#include "OpencvDecoder.h"

OpencvDecoder::OpencvDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id)
    , _video()
{
}

OpencvDecoder::~OpencvDecoder()
{
}

void OpencvDecoder::Create() throw(BaseException)
{
    // start video with OPENCV
    char* pEnd;
    long interval;
    interval = strtol(_url.c_str(), &pEnd, 10);
    if (ERANGE == errno)
    {
        throw BaseException(0, "USB port(" + _url + ") is not valid");
    }
    else
    {
        _video.open(static_cast<int>(interval));
    }

    // if video open failed
    if (!_video.isOpened())
    {
        throw BaseException(0, "USB(" + _url + ") can not be opened");
    }

    // create decode thread
    BaseDecoder::Create();
}

void OpencvDecoder::Destroy() throw(BaseException)
{
    _video.release();

    // destroy decode thread
    BaseDecoder::Destroy();
}

bool OpencvDecoder::ReadFrame(cv::Mat& frame)
{
    int readFailCount = 0;
    while (!_video.read(frame)) {
        if (++readFailCount >= 5) {
            printf("%s failed[%d times]\n", __FUNCTION__, readFailCount);
            return false;
        }
    }
    return true;
}

bool OpencvDecoder::ReadFrame(cv::cuda::GpuMat& frame)
{
    cv::Mat mat;
    if (ReadFrame(mat))
    {
        frame.upload(mat);
        return true;
    }
    else
    {
        return false;
    }
}


