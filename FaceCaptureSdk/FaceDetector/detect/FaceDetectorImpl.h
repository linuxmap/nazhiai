#ifndef _FACEDETECTOR_IMPLEMENT_HEADER_H_
#define _FACEDETECTOR_IMPLEMENT_HEADER_H_

#include "FaceExtractorImpl.h"

#include "SnapStruct.h"

typedef std::vector<int> ResolutionIndex;
typedef std::vector<int> Intervals;
typedef std::map<int, ResolutionIndex> ResolutionGroup;

typedef std::vector<ApiImagePtr> ApiImagePtrBatch;
typedef std::list<ApiImagePtr> ApiImagePtrBuffer;

typedef BestFinder<int, FaceBox, std::string> FaceAttriFinder;

struct FaceStat
{
    int trackId = -1;
    int trackedFrameNumber = 0;
};
typedef BestFinder<int, FaceStat, std::string> FaceStatFinder;
void ResetFaceStat(FaceStat& faceStat);

class SnapMachine;
class FaceDetector
{
private:
    class Worker
    {
    public:
        Worker(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
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
        FaceDetector& _manager;
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

    class Detector : public Worker
    {
    public:
        Detector(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~Detector();

    protected:
        void PreHeat();
        void Work();

        bool DetectFacesByResolutionGroup(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& trackApiImageIPtrBuffer, size_t& trackSize, ApiImagePtrBuffer& nextApiImageIPtrBuffer, size_t& nextSize);

    private:
        Detector(const Detector&);
        Detector& operator=(const Detector&);

    };
    typedef std::shared_ptr<Detector> DetectorPtr;
    typedef std::vector<DetectorPtr> DetectorPtrs;

    class Tracker : public Worker
    {
    public:
        Tracker(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~Tracker();

    protected:
        void PreHeat();
        void Work();

        bool TrackOne(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& trackedApiImageIPtrBuffer, size_t& trackedSize);

    private:
        Tracker(const Tracker&);
        Tracker& operator=(const Tracker&);

    };
    typedef std::shared_ptr<Tracker> TrackerPtr;
    typedef std::vector<TrackerPtr> TrackerPtrs;

    class Evaluator : public Worker
    {
    public:
        Evaluator(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~Evaluator();

    protected:
        void Work();

        bool FilterFaces(ApiImagePtrBatch& apiImageIPtrBatch, MultiFaceSdkBoxes& multiFaceSdkBoxes, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize);

        bool EvaluateOneBadness(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize);

    private:
        Evaluator(const Evaluator&);
        Evaluator& operator=(const Evaluator&);
    };
    typedef std::shared_ptr<Evaluator> EvaluatorPtr;
    typedef std::vector<EvaluatorPtr> EvaluatorPtrs;

    class KeyPointer : public Worker
    {
    public:
        KeyPointer(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~KeyPointer();

    protected:
        void PreHeat();
        void Work();
        
        bool FilterFaces(ApiImagePtrBatch& apiImageIPtrBatch, MultiFaceSdkBoxes& multiFaceSdkBoxes, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize);

        bool DeteckKeypointsOne(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize);

    private:
        KeyPointer(const KeyPointer&);
        KeyPointer& operator=(const KeyPointer&);

    };
    typedef std::shared_ptr<KeyPointer> KeyPointerPtr;
    typedef std::vector<KeyPointerPtr> KeyPointerPtrs;

    class Aligner : public Worker
    {
    public:
        Aligner(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~Aligner();

    protected:
        void PreHeat();
        void Work();

        bool AlignOne(ApiImagePtrBatch& apiImageIPtrBatch, AnalyzeResultPtrBuffer& toExtractResultPtrBuffer, size_t& toExtractResultPtrBufferSize,
            AnalyzeResultPtrBuffer& toAnalyzeResultPtrBuffer, size_t& toAnalyzeResultPtrBufferSize, CaptureResults& captureResults);

    private:
        CaptureResultPtr GenerateCaptureResult(ApiImagePtr& apiImagePtr, const FaceBox& faceBox);
        CaptureResultPtr GenerateAnalyzeResult(int devIndex, CaptureResultPtr& capture, FaceSdkImage* aligned, const FaceParam& faceParam);

    private:
        cv::cuda::Stream* _stream;

    private:
        Aligner(const Aligner&);
        Aligner& operator=(const Aligner&);

    };
    typedef std::shared_ptr<Aligner> AlignerPtr;
    typedef std::vector<AlignerPtr> AlignerPtrPtrs;

    class FaceAttrAnalyzer : public Worker
    {
    public:
        FaceAttrAnalyzer(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam);
        ~FaceAttrAnalyzer();

    protected:
        void PreHeat();
        void Work();

        void AnalyzeOne(AnalyzeResultPtrBatch& analyzedResultPtrs, AnalyzeResultPtrBuffer& toExtractResultPtrBuffer, size_t& toExtractResultPtrBufferSize, CaptureResults& captureResults);

    private:
        FaceAttrAnalyzer(const FaceAttrAnalyzer&);
        FaceAttrAnalyzer& operator=(const FaceAttrAnalyzer&);

    };
    typedef std::shared_ptr<FaceAttrAnalyzer> FaceAttrAnalyzerPtr;
    typedef std::vector<FaceAttrAnalyzerPtr> FaceAttrAnalyzerPtrs;

public:
    FaceDetector(const ModelParam& modelParam, const DetectParam& detectParam, const TrackParam& trackParam, 
        const EvaluateParam& evaluateParam, const KeypointParam& keypointParam, const AlignParam& alignParam, 
        const AnalyzeParam& analyzerParam, const ResultParam& resultParam, FaceExtractor*);
    ~FaceDetector();

    void Start() throw(BaseException);
    void Stop();

    void AddDecoder(BaseDecoder* baseDecoder, const FaceParam& faceParam);
    void DeleteDecoder(BaseDecoder* baseDecoder);
    
    void DelFaceExtractor();

    bool GetCapture(CaptureResults& captureResults);

    int GetDeviceIndex();

private:
    ApiImagePtr FromDecodedFrame(DecodedFrame& decodeFrame, const FaceParam& faceParam);

    void InitializingCurrentState();
    
    void NotifyMe(int err);
    void WaitNotify(const char*) throw(BaseException);

    void StartOneDetector(int gpuIndex) throw(BaseException);
    void StopDetectors();

    void StartOneTracker(int gpuIndex) throw(BaseException);
    void StopTrackers();

    void StartOneBadnessEvalutor(int gpuIndex) throw(BaseException);
    void StopBadnessEvalutors();

    void StartOneKeypointer(int gpuIndex) throw(BaseException);
    void StopKeypointers();

    void StartOneAligner(int gpuIndex) throw(BaseException);
    void StopAligners();

    void StartOneAnalyzer(int gpuIndex) throw(BaseException);
    void StopAnalyzers();

    size_t PushBuffer(ApiImagePtrBuffer& buffer, size_t& size, int threshold, ApiImagePtrBuffer& addedBuffer, int addedSize);
    bool FetchBatch(ApiImagePtrBuffer& buffer, size_t& size, int threshold, ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneDetect(ApiImagePtrBuffer& apiImageIPtrBuffer, int size);
    bool FetchOneDetect(ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneTrack(ApiImagePtrBuffer& apiImageIPtrBuffer, int size);
    bool FetchOneTrack(ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneEvaluates(ApiImagePtrBuffer& apiImageIPtrBuffer, int size);
    bool FetchOneEvaluates(ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneDetectKeypoints(ApiImagePtrBuffer& apiImageIPtrBuffer, int size);
    bool FetchOneDetectKeypoints(ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneAlign(ApiImagePtrBuffer& apiImageIPtrBuffer, int size);
    bool FetchOneAlign(ApiImagePtrBatch& apiImageIPtrBatch);

    void PushOneAnalyze(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size);
    bool FetchOneAnalyze(AnalyzeResultPtrBatch& analyzeResultPtrs);

    void PushOneExtract(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size);

    void PushOneResults(CaptureResults& captureResults);

    void PushBests(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer);
private:
    bool _started;
    volatile int _error_code;

private:
    std::mutex _contextLocker;
    BaseDecoders _baseDecoders;

private:
    void StartPrepareDetectBuffer();
    void PrepareDetectBuffer();
    void StopPrepareDetectBuffer();

    std::thread _prepareDetectBuffer;
    bool _preparingDetectBuffer;

private:
    std::condition_variable _oneWorkerReady;
    std::mutex _oneWorkerReadyLocker;

private:
    ModelParam _modelParam;
    ResultParam _resultParam;
    FaceSdkParam _channelParam;

    FaceExtractor* _faceExtractor;
    
private:
    DetectParam _detectParam;
    bool _updateDetectBatchSizeDynamic;
    DetectorPtrs _detectors;
    std::condition_variable _detectBufferCondition;
    std::mutex _detectBufferLocker;
    ApiImagePtrBuffer _detectBuffer;
    size_t _detectImageNumber;

private:
    TrackParam _trackParam;
    bool _updateTrackBatchSizeDynamic;
    TrackerPtrs _trackers;
    std::condition_variable _trackBufferCondition;
    std::mutex _trackBufferLocker;
    ApiImagePtrBuffer _trackBuffer;
    size_t _trackImageNumber;

private:
    EvaluateParam _evaluateParam;
    bool _updateEvaluateBatchSizeDynamic;
    EvaluatorPtrs _evaluators;
    std::condition_variable _evaluateBufferCondition;
    std::mutex _evaluateBufferLocker;
    ApiImagePtrBuffer _evaluateBuffer;
    size_t _evaluateImageNumber;

private:
    KeypointParam _keypointParam;
    bool _updateKeypointBatchSizeDynamic;
    KeyPointerPtrs _keypointers;
    std::condition_variable _keyPointsBufferCondition;
    std::mutex _keyPointsBufferLocker;
    ApiImagePtrBuffer _keyPointsBuffer;
    size_t _keypointsImageNumber;

private:
    AlignParam _alignParam;
    bool _updateAlignBatchSizeDynamic;
    AlignerPtrPtrs _aligners;
    std::condition_variable _alignBufferCondition;
    std::mutex _alignBufferLocker;
    ApiImagePtrBuffer _alignBuffer;
    size_t _alignImageNumber;

private:
    AnalyzeParam _analyzeParam;
    FaceAttrAnalyzerPtrs _faceAttrAnalyzerPtrs;
    std::condition_variable _faceAttrAnalyzeBufferCondition;
    std::mutex _faceAttrAnalyzeBufferLocker;
    AnalyzeResultPtrBuffer _faceAttrAnalyzeBuffer;
    size_t _faceNumberToAnalyze;

private:
    std::mutex _outputBufferLocker;
    CaptureResultsQueue _outputBuffer;
    long long _resultFaceNumber;

private:
    FaceStatFinder _faceStatFinder;
    FaceAttriFinder _faceAttriFinder;

private:
    static void BestFaceFinderCallback(void*, AnalyzeResultPtrBuffer& analyzedResultPtrBuffer);
    BestFaceFinder _bestFaceFinder;

private:
    FaceDetector();
    FaceDetector(const FaceDetector&);
    FaceDetector& operator=(const FaceDetector&);

    friend class SnapMachine;
};

#endif

