#ifndef _BASEDECODER_HEADER_H_
#define _BASEDECODER_HEADER_H_

#include "FaceDetectStruct.h"
#include "StreamDecodeStruct.h"
#include "BaseException.h"
#include "DecodedFrameQueue.h"

#include "FPS.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#pragma warning(disable:4290)

class CallbackPool
{
public:
    typedef void(*StoppedCallback)(const std::string& id, bool async);

    typedef struct  
    {
        std::string id;
        char async;
        StoppedCallback callback;
    } CallbackNode;

public:
    static void InitializeCallbackContext();
    static void AddCallback(const std::string& id, bool async, StoppedCallback cb);
    static void DestroyCallbackContext();

protected:
    static void Loop();

protected:
    static volatile bool _looping;
    static std::thread _loop;
    static std::mutex _locker;
    static std::condition_variable _condition;
    static std::queue<CallbackNode> _callbacks;

private:
    CallbackPool();
    CallbackPool(const CallbackPool&);
    CallbackPool& operator=(const CallbackPool&);
};

class BaseDecoder
{
public:
    BaseDecoder(const std::string&, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~BaseDecoder();
    
    virtual void Create() throw(BaseException);
    virtual void Destroy();

    virtual const char* Name() const
    {
        return "BaseDecoder";
    }

    virtual bool IsAsync()
    {
        return false;
    }

    inline const std::string& Id() const
    {
        return _id;
    }

    inline const int GetDeviceIndex() const
    {
        return _decoderParam.device_index;
    }

    inline void SetStoppedCallback(CallbackPool::StoppedCallback cb)
    {
        _stoppedCallback = cb;
    }

    bool GetFrame(DecodedFrame& decodedFrame, bool sync = false);

    bool DecodeFrame();

    inline void SetFaceParam(const FaceParam& faceParam) { _faceParam = faceParam; }
    inline const FaceParam& GetFaceParam() const { return _faceParam; }

protected:
    virtual bool Init();
    virtual void Uninit();

    virtual bool ReadFrame(DecodedFrame& decodedFrame);

protected:
    void WaitResult();
    void NotifyResult();
    void Decode();

    bool CanFrameBeUsed(int& framePosition, DecodedFrame& decodedFrame);
    bool CanFrameBeUsed(int& framePosition, const std::vector<char>& frame);
    bool CanFrameBeUsed(int& framePosition, const cv::Mat& frame);
    bool CanFrameBeUsed(int& framePosition, const cv::cuda::GpuMat& frame);

    bool ReviseFrame(DecodedFrame& decodedFrame);
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
    volatile bool _decoding;
    std::mutex _decoderLocker;

    std::mutex _syncLocker;
    std::condition_variable _syncCondition;
    std::string _errorMessage;

    int          _userFrameInterval;
    double       _origFrameInterval;

    int          _currentSkipPosition;
    unsigned long long _nextFrameId;

    clock_t      _failureStart;
    int          _failureDuration;
    int          _restartTimes;

    bool _buffered;

    FaceParam _faceParam;
    int _detectFramePos;

    DecodedFrameQueue _decodedFrameQueue;

    CallbackPool::StoppedCallback _stoppedCallback;

    FPStat fpstat;

private:
    BaseDecoder();
    BaseDecoder(const BaseDecoder&);
    BaseDecoder& operator=(const BaseDecoder&);
};
typedef std::vector<BaseDecoder*> BaseDecoders;

#endif

