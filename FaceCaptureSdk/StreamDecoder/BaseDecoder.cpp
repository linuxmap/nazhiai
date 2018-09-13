
#include "BaseDecoder.h"
#include "ImageProcess.h"
#include "AutoLock.h"

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


DecodedFrame::DecodedFrame(const std::string& streamId, unsigned int frameId)
    : sourceId(streamId), id(frameId), timestamp(TimeStamp<MILLISECONDS>::Now())
{
}

DecodedFrame::~DecodedFrame()
{
}

BaseDecoder::BaseDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : _url(url), _id(id)
    , _decoderParam(decoderParam), _decodeParam(decodeParam)
    , _decodeThread(), _decoding(false)
    , _msPerFrame(0), _msPerFrameLocker(), _msPerFrameCondition()
    , _currentSkipPosition(0), _nextFrameId(0)
    , _queueLocker(), _queueCondition(), _decodeFrameQueue()
{
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
    }
}

void BaseDecoder::Destroy() throw(BaseException)
{
    if (_decoding)
    {
        _decoding = false;
        WAIT_TO_EXIT(_decodeThread);
    }
}

DecodedFramePtr BaseDecoder::GetFrame()
{
    DecodedFramePtr decodedFramePtr(nullptr);
    if (_decoderParam.synchronize)
    {
        // set timeout for waiting to avoiding block forever
        WAIT_MILLISEC_TILL_COND(_queueCondition, _queueLocker, 1000, [this]{ return _decodeFrameQueue.size() > 0; });
        if (_decodeFrameQueue.size() > 0)
        {
            decodedFramePtr = _decodeFrameQueue.front();
            _decodeFrameQueue.pop();
        }
    } 
    else
    {
        AUTOLOCK(_queueLocker);
        if (_decodeFrameQueue.size() > 0)
        {
            decodedFramePtr = _decodeFrameQueue.front();
            _decodeFrameQueue.pop();
        } 
    }
    return decodedFramePtr;
}

void BaseDecoder::Decode()
{
    while (_decoding)
    {
        if (_msPerFrame > 0)
        {
            WAIT_MILLISEC(_msPerFrameCondition, _msPerFrameLocker, _msPerFrame);
        }

        if (_decoderParam.device_index >= 0)
        {
            cv::cuda::GpuMat frame;
            if (ReadFrame(frame) && CanFrameBeUsed(_currentSkipPosition, frame))
            {
                AUTOLOCK(_queueLocker);
                _decodeFrameQueue.push(DecodedFramePtr(new GpuMatFrame(_id, NextFrameId(), frame)));
                if (_decodeFrameQueue.size() > _decoderParam.buffer_size)
                {
                    _decodeFrameQueue.pop();
                }
            }
        } 
        else
        {
            cv::Mat frame;
            if (ReadFrame(frame) && CanFrameBeUsed(_currentSkipPosition, frame))
            {
                AUTOLOCK(_queueLocker);
                _decodeFrameQueue.push(DecodedFramePtr(new MatFrame(_id, NextFrameId(), frame)));
                if (_decodeFrameQueue.size() > _decoderParam.buffer_size)
                {
                    _decodeFrameQueue.pop();
                }
            }
        }
    }
}

unsigned int BaseDecoder::NextFrameId()
{
    return _nextFrameId++;
}

bool BaseDecoder::CanFrameBeUsed(int& framePosition, const cv::Mat& frame)
{
    if (_decodeParam.skip_frame_size == 0)
    {
        return !frame.empty();
    }

    if (framePosition++ == 0)
    {
        return !frame.empty();
    }

    if (framePosition < _decodeParam.skip_frame_size)
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
    if (_decodeParam.skip_frame_size == 0)
    {
        return !frame.empty();
    }

    if (framePosition++ == 0)
    {
        return !frame.empty();
    }

    if (framePosition < _decodeParam.skip_frame_size)
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



