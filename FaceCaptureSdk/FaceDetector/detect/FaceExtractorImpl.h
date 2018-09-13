
#ifndef _FACEEXTRACTORIMPL_HEADER_H_
#define _FACEEXTRACTORIMPL_HEADER_H_

#include "FaceSdkApi.h"
#include "FaceDetectCore.h"
#include "FaceDetectStruct.h"
#include "Performance.h"
#include "Finder.h"
#include "BaseDecoder.h"

typedef BestFinder<int, AnalyzeResultPtr, std::string> BestFaceFinder;

void ResetAnalyzeResultPtr(AnalyzeResultPtr& analyzeResultPtr);

class FaceExtractor
{
private:
    class Worker
    {
    public:
        Worker(FaceExtractor& manager, int gpuIndex, FaceSdkParam& channelParam);
        virtual ~Worker();

    public:
        int Start(int ctxIndex = 0);
        void ReadyToStop();
        int Stop();

    protected:
        int CreateThreadChannel();

        void Execute();

        virtual void Work();

    protected:
        FaceExtractor& _manager;
        int _gpuIndex;
        int _ctxIndex;

        std::thread _worker;
        char _name[128];
        bool _working;

        FaceSdkParam _channelParam;
        FaceSdkChannel* _channel;

    private:
        Worker();
        Worker(const Worker&);
        Worker& operator=(const Worker&);

    };
    typedef std::shared_ptr<Worker> WorkerPtr;

    class Extractor : public Worker
    {
    public:
        Extractor(FaceExtractor& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~Extractor();

    protected:
        void Work();

        bool ExtractOne(AnalyzeResultPtrBatch& analyzedResultPtrs, CaptureResults& captureResults);

    private:
        Extractor(const Extractor&);
        Extractor& operator=(const Extractor&);

    };
    typedef std::shared_ptr<Extractor> ExtractorPtr;
    typedef std::vector<ExtractorPtr> ExtractorPtrs;

public:
    FaceExtractor(const ModelParam& modelParam, const ExtractParam& extractParam, const ResultParam& resultParam);
    ~FaceExtractor();

    void Start() throw(BaseException);
    void Stop();

    bool GetCapture(CaptureResults& captureResults);

    void PushOneExtract(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size);

public:
    static size_t PushBuffer(AnalyzeResultPtrBuffer& buffer, size_t& size, int threshold, AnalyzeResultPtrBuffer& addedBuffer, int addedSize);
    static bool FetchBatch(AnalyzeResultPtrBuffer& buffer, size_t& size, int threshold, AnalyzeResultPtrBatch& apiImageIPtrBatch);

private:
    void InitializingCurrentState();

    void NotifyMe(int err);
    void WaitNotify(const char*) throw(BaseException);

    void StartOneExtractor(int gpuIndex) throw(BaseException);
    void StopExtractors();

    bool FetchOneExtract(AnalyzeResultPtrBatch& analyzeResultPtrs);

    void PushOneResults(CaptureResults& captureResults);

private:
    bool _started;

    volatile int _error_code;

private:
    std::condition_variable _oneWorkerReady;
    std::mutex _oneWorkerReadyLocker;

private:
    ModelParam _modelParam;
    ResultParam _resultParam;
    ExtractParam _extractParam;
    FaceSdkParam _channelParam;

private:
    ExtractorPtrs _extractorPtrs;
    std::condition_variable _extractBufferCondition;
    std::mutex _extractBufferLocker;
    AnalyzeResultPtrBuffer _extractBuffer;
    size_t _faceNumberToExtract;

private:
    std::mutex _outputBufferLocker;
    CaptureResultsQueue _outputBuffer;
    long long _resultFaceNumber;

private:
    FaceExtractor();
    FaceExtractor(const FaceExtractor&);
    FaceExtractor& operator=(const FaceExtractor&);
};

#endif

