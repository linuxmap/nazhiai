#include "FaceExtractorImpl.h"

#include "TimeStamp.h"

#include "FPS.h"
#include "SnapMachine.h"
#include "DecodeManager.h"

#include "GpuCtxIndex.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#pragma warning(disable:4267)

void ResetAnalyzeResultPtr(AnalyzeResultPtr& analyzeResultPtr)
{
    analyzeResultPtr = nullptr;
}

FaceExtractor::Worker::Worker(FaceExtractor& manager, int gpuIndex, FaceSdkParam& channelParam)
    : _manager(manager), _gpuIndex(gpuIndex), _ctxIndex(0)
    , _worker(), _working(false)
    , _channelParam(channelParam), _channel(nullptr)
{
    memset(_name, 0, sizeof(_name));
}

FaceExtractor::Worker::~Worker()
{
    if (_channel)
    {
        DestroyChannel(_channel);
        _channel = nullptr;
    }
}

int FaceExtractor::Worker::Start(int ctxIndex)
{
    if (!_working)
    {
        _ctxIndex = ctxIndex;
        _working = true;
        _worker = std::thread(&Worker::Execute, this);
        return 0;
    }
    return -1;
}

void FaceExtractor::Worker::ReadyToStop()
{
    if (_working)
    {
        _working = false;
    }
}

int FaceExtractor::Worker::Stop()
{
    WAIT_TO_EXIT(_worker);
    return 0;
}

int FaceExtractor::Worker::CreateThreadChannel()
{
    FaceSdkResult res = FaceSdkOk;

    // if set GPU unit index, then initialize GPU information
    if (_gpuIndex > -1)
    {
        int gpuCount = 0;
        FaceSdkResult res = GetGpuCount(gpuCount);
        if (res == FaceSdkOk)
        {
            if (gpuCount > 0)
            {
                if (_gpuIndex >= gpuCount)
                {
                    LOG(ERROR) << _name << "invalid GPU index: " << _gpuIndex << ", and the default GPU index will be 0";
                    return -2;
                }

                res = SetGpuForThread(_gpuIndex, _ctxIndex);
                if (res != FaceSdkOk)
                {
                    LOG(ERROR) << _name << "set GPU for thread failed, error code: " << res;
                    return res;
                }
            }
            else
            {
                LOG(ERROR) << _name << "no GPU unit detected";
                return -1;
            }
        }
        else
        {
            LOG(ERROR) << _name << "detect GPU failed, error code: " << res;
            return res;
        }
    }

    // create channel
    res = CreateChannel(_channel, _channelParam);
    if (res != FaceSdkOk)
    {
        LOG(ERROR) << _name << "Create channel failed, error code: " << res;
        return res;
    }

    LOG(INFO) << _name << " working GPU will be GPU:" << _gpuIndex << "-" << _ctxIndex;;

    return res;
}

void FaceExtractor::Worker::Execute()
{
    _manager.InitializingCurrentState();

    // configure working device and create channel
    int err = CreateThreadChannel();
    if (0 == err)
    {
        // notify the manager to get the result
        _manager.NotifyMe(err);

        LOG(INFO) << _name << ":(" << std::this_thread::get_id() << ") ready to work";

        // start to work
        while (_working)
        {
            Work();
        }

        LOG(INFO) << _name << "(" << std::this_thread::get_id() << ") stop work";
        DestroyChannel(_channel);
        _channel = nullptr;
    }
    else
    {
        _manager.NotifyMe(err);
    }
}

void FaceExtractor::Worker::Work()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

FaceExtractor::Extractor::Extractor(FaceExtractor& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableExtract = true;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceExtractor::Extractor::~Extractor()
{

}

void FaceExtractor::Extractor::Work()
{
    AnalyzeResultPtrBatch analyzedResultPtrBatch;
    START_EVALUATE(FetchOneExtract);
    if (_manager.FetchOneExtract(analyzedResultPtrBatch))
    {
        PRINT_COSTS(FetchOneExtract);

        START_FUNCTION_EVALUATE();

        CaptureResults captureResults;
        START_EVALUATE(ExtractOne);
        if (ExtractOne(analyzedResultPtrBatch, captureResults))
        {
            PRINT_COSTS(ExtractOne);

            if (captureResults.size() > 0)
            {
                START_EVALUATE(PushOneResults);
                _manager.PushOneResults(captureResults);
                PRINT_COSTS(PushOneResults);
            }
        }

        PRINT_FUNCTION_COSTS();
    }
}

bool FaceExtractor::Extractor::ExtractOne(AnalyzeResultPtrBatch& analyzedResultPtrs, CaptureResults& captureResults)
{
    DECLARE_FPS_STATIC(5000);

    int devIndex = _manager._extractParam.deviceIndex;
    FaceSdkImages alignedFaceSdkImages;
    for (size_t idxBefore = 0; idxBefore < analyzedResultPtrs.size(); ++idxBefore)
    {
        AnalyzeResultPtr& analyzeResultPtr = analyzedResultPtrs[idxBefore];
        if (analyzeResultPtr->CreateSdkImage(devIndex))
        {
            alignedFaceSdkImages.push_back(analyzeResultPtr->alignedImage);
        }
    }

    // check if has any aligned SDK image
    if (alignedFaceSdkImages.size() == 0)
    {
        return false;
    }

    Features features;
    std::vector<int> ages;
    std::vector<float> genders;
    START_EVALUATE(ExtractFeatures);
    if (ExtractFeatures(_channel, alignedFaceSdkImages, features, ages, genders) != FaceSdkOk)
    {
        return false;
    }
    PRINT_COSTS(ExtractFeatures);

    PRINT("%s(%p) extract face number: %d\n", __FUNCTION__, &_manager, alignedFaceSdkImages.size());
    STATISTIC_FPS(alignedFaceSdkImages.size(), __FUNCTION__);

    size_t featureSize = (size_t)(features.size() / alignedFaceSdkImages.size());
    size_t featurePrevPos = 0, featureNextPos = featurePrevPos;
    for (size_t idxAfter = 0; idxAfter < analyzedResultPtrs.size(); ++idxAfter)
    {
        AnalyzeResultPtr& analyzeResultPtr = analyzedResultPtrs[idxAfter];

        const FaceParam& faceParam = analyzeResultPtr->faceParam;

        CaptureResultPtr& captrueResultPtr = analyzeResultPtr->captureResultPtr;
        std::vector<char>& feature = captrueResultPtr->faceBox.feature;
        if (analyzeResultPtr->alignedImage)
        {
            featureNextPos += featureSize;
            feature.insert(feature.end(), features.begin() + featurePrevPos, features.begin() + featureNextPos);
            featurePrevPos = featureNextPos;
        }

        FaceBox& currentFaceBox = captrueResultPtr->faceBox;

        currentFaceBox.age = ages[idxAfter];
        currentFaceBox.gender = genders[idxAfter] > 0.50f ? 0 : 1;
        captureResults.push_back(captrueResultPtr);
    }

    return true;
}

FaceExtractor::FaceExtractor(const ModelParam& modelParam, const ExtractParam& extractParam, const ResultParam& resultParam)
    : _started(false), _error_code(0)
    , _oneWorkerReady(), _oneWorkerReadyLocker()
    , _modelParam(modelParam), _resultParam(resultParam), _extractParam(extractParam), _channelParam()
    , _extractorPtrs(), _extractBufferCondition(), _extractBufferLocker(), _extractBuffer(), _faceNumberToExtract(0)
    , _outputBufferLocker(), _outputBuffer(), _resultFaceNumber(0)
{
    _channelParam.featureModel = _modelParam.name;
    _channelParam.modelDir = _modelParam.path;
}


FaceExtractor::~FaceExtractor()
{
}

void FaceExtractor::Start() throw(BaseException)
{
    if (!_started)
    {
        _started = true;

        try
        {
            if (_extractParam.deviceIndex >= 0)
            {
                StartOneExtractor(_extractParam.deviceIndex);
            }
            else
            {
                StartOneExtractor(-1);
            }
        }
        catch (BaseException& ex)
        {
            Stop();
            throw ex;
        }
    }
}

void FaceExtractor::Stop()
{
    if (_started)
    {
        _started = false;

        StopExtractors();
    }
}

bool FaceExtractor::GetCapture(CaptureResults& captureResults)
{
    AUTOLOCK(_outputBufferLocker);
    while (_outputBuffer.size() > 0)
    {
        CaptureResults& alias = _outputBuffer.front();
        captureResults.insert(captureResults.end(), alias.begin(), alias.end());
        _resultFaceNumber -= alias.size();

        _outputBuffer.pop();
    }

    return captureResults.size() > 0;
}

void FaceExtractor::StartOneExtractor(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _extractParam.threadCount; ++idx)
    {
        ExtractorPtr extractorPtr(new Extractor(*this, gpuIndex, _channelParam));
        if (extractorPtr)
        {
            extractorPtr->Start(GpuCtxIndex::Next(gpuIndex));

            WaitNotify(__FUNCTION__);

            _extractorPtrs.push_back(extractorPtr);
        }
    }
}

void FaceExtractor::StopExtractors()
{
    for each (ExtractorPtr extractorPtr in _extractorPtrs)
    {
        extractorPtr->ReadyToStop();
        extractorPtr->Stop();
    }
    _extractorPtrs.clear();
}

void FaceExtractor::PushOneExtract(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size)
{
    AUTOLOCK(_extractBufferLocker);
    long long poppedSize = PushBuffer(_extractBuffer, _faceNumberToExtract, _extractParam.bufferSize, analyzeResultPtrBuffer, size);
    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " faces in extract on Gpu:" << _extractParam.deviceIndex << " were discarded, because of buffer overflow: " << _extractParam.bufferSize;
    }

    WAKEUP_ALL(_extractBufferCondition);
}

bool FaceExtractor::FetchOneExtract(AnalyzeResultPtrBatch& analyzeResultPtrs)
{
    WAIT_MILLISEC_TILL_COND(_extractBufferCondition, _extractBufferLocker, _extractParam.batchTimeout, [this](){ return _faceNumberToExtract >= _extractParam.batchSize; });

    if (_faceNumberToExtract > 0)
    {
        return FaceExtractor::FetchBatch(_extractBuffer, _faceNumberToExtract, _extractParam.batchSize, analyzeResultPtrs);
    }

    return false;
}

void FaceExtractor::PushOneResults(CaptureResults& captureResults)
{
    if (captureResults.size() > 0)
    {
        AUTOLOCK(_outputBufferLocker);
        _outputBuffer.push(captureResults);
        _resultFaceNumber += captureResults.size();

        long long poppedSize = 0;
        while (_outputBuffer.size() > 0 && _resultFaceNumber > _resultParam.bufferSize)
        {
            long long headSize = _outputBuffer.front().size();
            _outputBuffer.pop();
            poppedSize += headSize;
        }

        if (poppedSize > 0)
        {
            _resultFaceNumber -= poppedSize;
            LOG(WARNING) << poppedSize << " faces were discarded in extract result buffer, because of buffer overflow: " << _resultParam.bufferSize;
        }
    }
}

size_t FaceExtractor::PushBuffer(AnalyzeResultPtrBuffer& buffer, size_t& size, int threshold, AnalyzeResultPtrBuffer& addedBuffer, int addedSize)
{
    size += addedSize;
    buffer.splice(buffer.end(), addedBuffer);

    size_t poppedSize = 0;
    if (size > threshold)
    {
        poppedSize = size - threshold;

        AnalyzeResultPtrBuffer::iterator newbegin = buffer.begin();
        std::advance(newbegin, poppedSize);
        buffer.erase(buffer.begin(), newbegin);

        size = threshold;
    }
    return poppedSize;
}

bool FaceExtractor::FetchBatch(AnalyzeResultPtrBuffer& buffer, size_t& size, int threshold, AnalyzeResultPtrBatch& apiImageIPtrBatch)
{
    size_t fetchedSize = size >= threshold ? threshold : size;

    AnalyzeResultPtrBuffer::iterator newbegin = buffer.begin();
    std::advance(newbegin, fetchedSize);

    std::copy(buffer.begin(), newbegin, std::back_inserter(apiImageIPtrBatch));
    buffer.erase(buffer.begin(), newbegin);

    size -= fetchedSize;

    return fetchedSize > 0;
}

void FaceExtractor::InitializingCurrentState()
{
    _error_code = 0;
}

void FaceExtractor::NotifyMe(int err)
{
    _error_code = err;
    WAKEUP_ONE(_oneWorkerReady);
}

void FaceExtractor::WaitNotify(const char* msg) throw(BaseException)
{
    // wait the working thread to wake up to stop wait
    WAIT(_oneWorkerReady, _oneWorkerReadyLocker);

    // if failed, then stop
    if (_error_code)
    {
        throw BaseException(_error_code, std::string(msg) + " failed, error code: " + std::to_string(_error_code));
    }
}



