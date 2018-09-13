
#include "OpencvDecoder.h"

#include "AutoLock.h"
#include "Performance.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

OpencvDecoder::OpencvDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id)
    , _video()
{
}

OpencvDecoder::~OpencvDecoder()
{
}

bool OpencvDecoder::Init()
{
    AUTOLOCK(_decoderLocker);
    // start video with OPENCV
    char* pEnd;
    long interval;
    interval = strtol(_url.c_str(), &pEnd, 10);
    if (ERANGE == errno)
    {
        _errorMessage = "USB port(" + _url + ") is not valid";
        LOG(INFO) << _errorMessage;
        return false;
    }
    else
    {
        _video.open(static_cast<int>(interval));
    }

    if (_decoderParam.width > 0)
    {
        _video.set(CV_CAP_PROP_FRAME_WIDTH, _decoderParam.width);
    }
    if (_decoderParam.height > 0)
    {
        _video.set(CV_CAP_PROP_FRAME_HEIGHT, _decoderParam.height);
    }

    _decodeParam.fps = (float)_video.get(CV_CAP_PROP_FPS);
    _origFrameInterval = 1000.0f / _decodeParam.fps;

    // if video open failed
    if (!_video.isOpened())
    {
        _errorMessage = "USB(" + _url + ") can not be opened";
        LOG(INFO) << _errorMessage;
        return false;
    }
    _errorMessage = "";
    LOG(INFO) << Name() << "(" << _id << ") create success";

    _nextFrameId = 0;

    return true;
}

void OpencvDecoder::Uninit()
{
    if (_video.isOpened())
    {
        _video.release();
        LOG(INFO) << Name() << "(" << _id << ") destroy success";
    }
    else
    {
        LOG(INFO) << Name() << "(" << _id << ") is not created yet";
    }
    _nextFrameId = 0;
}

bool OpencvDecoder::ReadFrame(DecodedFrame& frame)
{
    if (!_video.read(frame.mat))
    {
        return false;
    }
    frame.position = (long long)(_origFrameInterval * _nextFrameId);
    frame.id = _nextFrameId++;
    return !frame.mat.empty();
}


