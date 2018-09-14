
#include "BaseDecoder.h"
#include "ImageProcess.h"
#include "AutoLock.h"

#include "TimeStamp.h"
#include "Performance.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#ifdef _DEBUG
#pragma comment(lib, "opencv_core320d.lib")
#pragma comment(lib, "opencv_imgcodecs320d.lib")
#pragma comment(lib, "opencv_imgproc320d.lib")
#pragma comment(lib, "opencv_videoio320d.lib")
#else
#pragma comment(lib, "opencv_core320.lib")
#pragma comment(lib, "opencv_imgcodecs320.lib")
#pragma comment(lib, "opencv_imgproc320.lib")
#pragma comment(lib, "opencv_videoio320.lib")
#endif

volatile bool CallbackPool::_looping = false;
std::thread CallbackPool::_loop;
std::mutex CallbackPool::_locker;
std::condition_variable CallbackPool::_condition;
std::queue<CallbackPool::CallbackNode> CallbackPool::_callbacks = {};

void CallbackPool::InitializeCallbackContext()
{
    _looping = true;
    _loop = std::thread(&CallbackPool::Loop);
}

void CallbackPool::AddCallback(const std::string& id, bool async, StoppedCallback cb)
{
    AUTOLOCK(_locker);

    if (!id.empty() && cb)
    {
        _callbacks.push(CallbackNode{id, async ? 1 : 0, cb});
        WAKEUP_ONE(_condition);
    }
}

void CallbackPool::DestroyCallbackContext()
{
    if (_looping)
    {
        _looping = false;
        WAIT_TO_EXIT(_loop);
    }
}

void CallbackPool::Loop()
{
    while (_looping)
    {
        WAIT_MILLISEC_TILL_COND(_condition, _locker, 500, []{ return _callbacks.size() > 0; });
        if (_callbacks.size() > 0)
        {
            CallbackNode& cbnode = _callbacks.front();
            if (cbnode.callback)
            {
                cbnode.callback(cbnode.id, 0 == cbnode.async ? false : true);
            }
            _callbacks.pop();
        }
    }
}


BaseDecoder::BaseDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : _url(url), _id(id)
    , _decoderParam(decoderParam), _decodeParam(decodeParam)
    , _decodeThread(), _decoding(false), _decoderLocker()
    , _syncLocker(), _syncCondition(), _errorMessage()
    , _userFrameInterval(0), _origFrameInterval(0.0f)
    , _currentSkipPosition(0), _nextFrameId(0), _failureStart(0), _restartTimes(0)
    , _buffered(false), _faceParam(), _detectFramePos(1)
    , _decodedFrameQueue(_decoderParam.device_index, _decoderParam.buffer_size)
    , _stoppedCallback(nullptr)
    , fpstat(5000)
{
    _userFrameInterval = (int)(_decodeParam.fps >= 1.0f ? (1000.0f / _decodeParam.fps) : 0);
}

BaseDecoder::~BaseDecoder()
{
}

void BaseDecoder::Create() throw(BaseException)
{
    if (!_decoding)
    {
        _decoding = true;
        _decodeThread = std::thread(&BaseDecoder::Decode, this);

        WaitResult();
    }
}

void BaseDecoder::Destroy()
{
    if (_decoding)
    {
        _decoding = false;
        WAIT_TO_EXIT(_decodeThread);

        // user call back
        if (_decodeParam.ExitCallback)
        {
            _decodeParam.ExitCallback(_id.c_str());
        }
    }
}

bool BaseDecoder::GetFrame(DecodedFrame& decodedFrame, bool sync)
{
    bool got = _decodedFrameQueue.Pop(decodedFrame, sync);
    if (got && _faceParam.detect_interval > 1)
    {
        if (_detectFramePos > 1)
        {
            decodedFrame.needDetect = false;
        }

        ++_detectFramePos;

        if (_detectFramePos > _faceParam.detect_interval)
        {
            _detectFramePos = 1;
        }
    }
    return got;
}

bool BaseDecoder::Init()
{
    AUTOLOCK(_decoderLocker);
    _errorMessage = "Decoder not implemented";
    return false;
}

void BaseDecoder::Uninit()
{
}

bool BaseDecoder::DecodeFrame()
{
    DecodedFrame decodedFrame;
    if (ReadFrame(decodedFrame))
    {
        // reset to zero, because it is not continuous
        _failureStart = 0;

        if (CanFrameBeUsed(_currentSkipPosition, decodedFrame) && ReviseFrame(decodedFrame))
        {
            decodedFrame.buffered = _buffered;
            decodedFrame.sourceId = _id;
            decodedFrame.timestamp = TimeStamp<MILLISECONDS>::Now();
            _decodedFrameQueue.Push(decodedFrame);
        }

        fpstat.Stat(1, _id);

        return true;
    }

    if (_failureStart <= 0)
    {
        _failureStart = clock();
    }

    if (clock() >= _decodeParam.failureThreshold + _failureStart)
    {
        std::string decoder_instance_name = Name();
        decoder_instance_name += "(" + _id + ")";

        bool restart = false;
        if (_decodeParam.restartTimes < 0)
        {
            restart = true;
        }
        else
        {
            if (_restartTimes < _decodeParam.restartTimes)
            {
                restart = true;
            }
            else if (_restartTimes == _decodeParam.restartTimes)
            {
                CallbackPool::AddCallback(_id, IsAsync(), _stoppedCallback);
                LOG(INFO) << decoder_instance_name << " reach restart threshold: " << _decodeParam.restartTimes << ", and is going to exit";
            }
        }

        if (restart)
        {
            LOG(INFO) << decoder_instance_name << " is going to restart...";

            Uninit();

            if (Init())
            {
                LOG(INFO) << decoder_instance_name << " restart success";
            }
            else
            {
                LOG(INFO) << decoder_instance_name << " restart failed: " << _errorMessage;
            }
            _failureStart = 0;
        }
        _restartTimes += 1;
    }

    return false;
}

bool BaseDecoder::ReadFrame(DecodedFrame& decodedFrame)
{
    return false;
}

void BaseDecoder::WaitResult()
{
    WAIT(_syncCondition, _syncLocker);
    if (!_errorMessage.empty())
    {
        throw BaseException(0, _errorMessage);
    }
}

void BaseDecoder::NotifyResult()
{
    WAKEUP_ONE(_syncCondition);
}

void BaseDecoder::Decode()
{
    if (Init())
    {
        // notify creating thread
        NotifyResult();

        // start decoding
        while (_decoding)
        {
            if (_userFrameInterval > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(_userFrameInterval));
            }

            DecodeFrame();
        }

        Uninit();
    }
    else
    {
        NotifyResult();
    }
}

bool BaseDecoder::CanFrameBeUsed(int& framePosition, const std::vector<char>& frame)
{
    if (_decodeParam.skip_frame_interval == 0)
    {
        return !frame.empty();
    }

    if (framePosition++ == 0)
    {
        return !frame.empty();
    }

    if (framePosition < _decodeParam.skip_frame_interval)
    {
        framePosition++;
        return false;
    }
    else
    {
        framePosition = 0;
        return false;
    }
}

bool BaseDecoder::CanFrameBeUsed(int& framePosition, const cv::Mat& frame)
{
    if (_decodeParam.skip_frame_interval == 0)
    {
        return !frame.empty();
    }

    if (framePosition++ == 0)
    {
        return !frame.empty();
    }

    if (framePosition < _decodeParam.skip_frame_interval)
    {
        framePosition++;
        return false;
    }
    else
    {
        framePosition = 0;
        return false;
    }
}

bool BaseDecoder::CanFrameBeUsed(int& framePosition, const cv::cuda::GpuMat& frame)
{
    if (_decodeParam.skip_frame_interval == 0)
    {
        return !frame.empty();
    }

    if (framePosition++ == 0)
    {
        return !frame.empty();
    }

    if (framePosition < _decodeParam.skip_frame_interval)
    {
        framePosition++;
        return false;
    }
    else
    {
        framePosition = 0;
        return false;
    }
}

bool BaseDecoder::CanFrameBeUsed(int& framePosition, DecodedFrame& decodedFrame)
{
    if (!decodedFrame.mat.empty())
    {
        return CanFrameBeUsed(framePosition, decodedFrame.mat);
    }
    else if (!decodedFrame.gpumat.empty())
    {
        return CanFrameBeUsed(framePosition, decodedFrame.gpumat);
    }
    else if (decodedFrame.imdata && !decodedFrame.imdata->empty())
    {
        return CanFrameBeUsed(framePosition, *(decodedFrame.imdata));
    }
    else
    {
        return false;
    }
}

bool BaseDecoder::ReviseFrame(DecodedFrame& decodedFrame)
{
    if (!decodedFrame.mat.empty())
    {
        cv::Mat orig = decodedFrame.mat;
        return ReviseFrame(orig, decodedFrame.mat, _decodeParam.rotate_angle);
    }
    else if (!decodedFrame.gpumat.empty())
    {
        cv::cuda::GpuMat orig = decodedFrame.gpumat;
        return ReviseFrame(orig, decodedFrame.gpumat, _decodeParam.rotate_angle);
    }
    else
    {
        return true;
    }
}

bool BaseDecoder::ReviseFrame(const cv::Mat& origin, cv::Mat& revised, double rotateAngle)
{
    if (fabs(rotateAngle) < 0.5)
    {
        revised = origin;
    }
    else
    {
        if (0 != nzimg::CImageProcess::rotateImage(origin, revised, rotateAngle)) {
            return false;
        }
    }
    return true;
}

bool BaseDecoder::ReviseFrame(const cv::cuda::GpuMat& origin, cv::cuda::GpuMat& revised, double rotateAngle)
{
    if (fabs(rotateAngle) < 0.5)
    {
        revised = origin;
    }
    else
    {
        if (0 != nzimg::CImageProcess::rotateImage(origin, revised, rotateAngle)) {
            return false;
        }
    }
    return true;
}



