
#ifndef _STREAMGROUP_HEADER_H_
#define _STREAMGROUP_HEADER_H_

#include "StreamDecodeStruct.h"
#include "FaceDetectStruct.h"

#include "DecodedFrame.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include <queue>

class FaceDetector;
class BaseDecoder;

class AsyncContext
{
private:
    enum { OPER_NONE = -1, OPER_ADD, OPER_DEL };
    typedef struct OperInfo_st
    {
        int operationType = OPER_NONE;
        int streamType = 0;
        std::string url = "";
        DecoderParam decoderParam = {};
        DecodeParam decodeParam = {};
        FaceParam faceParam = {};
        std::string id = ""; 
        
        void(*asyncCallback)(const std::string&, const std::string&) = nullptr;
        void(*stoppedCallback)(const std::string&, bool async) = nullptr;
    } OperInfo;
    typedef std::queue<OperInfo> OperInfoQueue;

public:
    static const std::string SUCCESS;
    static const std::string FAILED;

    static std::string FailedReason(const std::string& des);

public:
    AsyncContext(int gpuIndex, FaceDetector* faceDetector);
    ~AsyncContext();

public:
    void Create();
    void Destroy();

    bool AddAsyncDecoder(int streamType, const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, const FaceParam& faceParam, void(*asyncCallback)(const std::string&, const std::string&), void(*stoppedCallback)(const std::string&, bool));
    bool DelAsyncDecoder(const std::string& id, void(*asyncCallback)(const std::string&, const std::string&));
    bool FindAsyncDecoder(const std::string& id);

private:
    void RetrieveFrame();

    void CreateAsyncDecoder(OperInfo& info);
    void DestroyAsyncDecoder(OperInfo& info);

private:
    FaceDetector* _faceDetector;
    int _gpuIndex;
    std::thread _retrieveThread;
    volatile bool _retrieving;

    std::vector<BaseDecoder*> _baseDecoders;

    std::mutex _operationsLocker;
    OperInfoQueue _operations;

private:
    AsyncContext();
    AsyncContext(const AsyncContext&);
    AsyncContext& operator=(const AsyncContext&);
};

#endif

