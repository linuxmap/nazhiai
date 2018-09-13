#ifndef _BASEDECODER_HEADER_H_
#define _BASEDECODER_HEADER_H_

#include "StreamDecodeStruct.h"
#include "BaseException.h"

#include "TimeStamp.h"

#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>

#include <queue>

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

class DecodedFrame 
{
public:
    std::string sourceId;
    unsigned int id;
    long long timestamp;

public:
    DecodedFrame(const std::string& streamId, unsigned int frameId);
    virtual ~DecodedFrame();

private:
    DecodedFrame();
    DecodedFrame(const DecodedFrame&);
    DecodedFrame& operator=(const DecodedFrame&);
};
typedef std::shared_ptr<DecodedFrame> DecodedFramePtr;

class MatFrame : public DecodedFrame
{
public:
    cv::Mat frame;

public:
    MatFrame(const std::string& streamId, unsigned int frameId, const cv::Mat& decodeMat)
        : DecodedFrame(streamId, frameId), frame(decodeMat)
    {}

    ~MatFrame()
    {}

private:
    MatFrame();
    MatFrame(const MatFrame&);
    MatFrame& operator=(const MatFrame&);
};
typedef std::shared_ptr<MatFrame> MatFramePtr;

class GpuMatFrame : public DecodedFrame
{
public:
    cv::cuda::GpuMat frame;

public:
    GpuMatFrame(const std::string& streamId, unsigned int frameId, const cv::cuda::GpuMat& decodeGpuMat)
        : DecodedFrame(streamId, frameId), frame(decodeGpuMat)
    {}

    ~GpuMatFrame()
    {}

private:
    GpuMatFrame();
    GpuMatFrame(const GpuMatFrame&);
    GpuMatFrame& operator=(const GpuMatFrame&);
};
typedef std::shared_ptr<GpuMatFrame> GpuMatFramePtr;

#pragma warning(disable:4290)

class BaseDecoder
{
public:
    BaseDecoder(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~BaseDecoder();
    
    virtual void Create() throw(BaseException);
    virtual void Destroy() throw(BaseException);

    inline virtual const char* Name() const
    {
        return "BaseDecoder_";
    }

    inline const std::string& Id() const
    {
        return _id;
    }

    DecodedFramePtr GetFrame();

protected:
    virtual bool ReadFrame(cv::Mat& frame) = 0;
    virtual bool ReadFrame(cv::cuda::GpuMat& frame) = 0;

protected:
    void Decode();

    unsigned int NextFrameId();

    bool CanFrameBeUsed(int& framePosition, const cv::Mat& frame);
    bool CanFrameBeUsed(int& framePosition, const cv::cuda::GpuMat& frame);

    bool ReviseFrame(const cv::Mat& origin, cv::Mat& revised, double rotateAngle);
    bool ReviseFrame(const cv::cuda::GpuMat& origin, cv::cuda::GpuMat& revised, double rotateAngle);

protected:
    // URL
    std::string _url;
    std::string _id;

    // decoder and decode parameter
    DecoderParam _decoderParam;
    DecodeParam _decodeParam;

    std::thread _decodeThread;
    bool _decoding;

    int _msPerFrame;
    std::mutex _msPerFrameLocker;
    std::condition_variable _msPerFrameCondition;

    int          _currentSkipPosition;
    unsigned int _nextFrameId;

    std::mutex _queueLocker;
    std::condition_variable _queueCondition;
    std::queue<DecodedFramePtr> _decodeFrameQueue;

private:
    BaseDecoder();
    BaseDecoder(const BaseDecoder&);
    BaseDecoder& operator=(const BaseDecoder&);
};
typedef std::shared_ptr<BaseDecoder> BaseDecoderPtr;

#endif

