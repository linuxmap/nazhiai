
#include "FaceDetectorImpl.h"

#include "TimeStamp.h"

#include "FPS.h"
#include "SnapMachine.h"
#include "DecodeManager.h"

#include "GpuCtxIndex.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#pragma warning(disable:4267)

static const float FLOAT_CONFIGURED = 0.0001f;

void ResetFaceStat(FaceStat& faceStat)
{
    faceStat.trackedFrameNumber = 0;
}

FaceDetector::Worker::Worker(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : _manager(manager), _gpuIndex(gpuIndex), _ctxIndex(0)
    , _worker(), _working(false)
    , _channelParam(channelParam), _channel(nullptr)
{
    memset(_name, 0, sizeof(_name));
}

FaceDetector::Worker::~Worker()
{
    if (_channel)
    {
        DestroyChannel(_channel);
        _channel = nullptr;
    }
}

int FaceDetector::Worker::Start(int ctxIndex)
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

void FaceDetector::Worker::ReadyToStop()
{
    if (_working)
    {
        _working = false;
    }
}

int FaceDetector::Worker::Stop()
{
    WAIT_TO_EXIT(_worker);
    return 0;
}

int FaceDetector::Worker::CreateThreadChannel()
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

    LOG(INFO) << _name << " working GPU will be GPU:" << _gpuIndex << "-" << _ctxIndex;

    return res;
}

void FaceDetector::Worker::Execute()
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

void FaceDetector::Worker::Work()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

FaceDetector::Detector::Detector(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableDetectFace = true;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::Detector::~Detector()
{
}

void FaceDetector::Detector::Work()
{
    ApiImagePtrBatch apiImageIPtrBatch;
    // we should detect images that have the same resolution at one time
    // that is the limitation of the SDK
    START_EVALUATE(FetchOneDetect);
    if (_manager.FetchOneDetect(apiImageIPtrBatch))
    {
        PRINT_COSTS(FetchOneDetect);

        START_FUNCTION_EVALUATE();

        ApiImagePtrBuffer trackApiImageIPtrBuffer, nextApiImageIPtrBuffer;
        size_t trackSize = 0, nextSize = 0;
        START_EVALUATE(DetectFacesByResolutionGroup);
        if (DetectFacesByResolutionGroup(apiImageIPtrBatch, trackApiImageIPtrBuffer, trackSize, nextApiImageIPtrBuffer, nextSize))
        {
            PRINT_COSTS(DetectFacesByResolutionGroup);

            if (trackSize > 0)
            {
                if (_manager._trackParam.threadCount > 0)
                {
                    START_EVALUATE(PushOneTrack);
                    _manager.PushOneTrack(trackApiImageIPtrBuffer, trackSize);
                    PRINT_COSTS(PushOneTrack);
                }
                else
                {
                    if (_manager._evaluateParam.threadCount > 0)
                    {
                        START_EVALUATE(PushOneEvaluates);
                        _manager.PushOneEvaluates(trackApiImageIPtrBuffer, trackSize);
                        PRINT_COSTS(PushOneEvaluates);
                    }
                    else
                    {
                        if (_manager._keypointParam.threadCount > 0)
                        {
                            START_EVALUATE(PushOneDetectKeypoints);
                            _manager.PushOneDetectKeypoints(trackApiImageIPtrBuffer, trackSize);
                            PRINT_COSTS(PushOneDetectKeypoints);
                        }
                    }
                }
            }

            if (nextSize > 0)
            {
                if (_manager._evaluateParam.threadCount > 0)
                {
                    START_EVALUATE(PushOneEvaluates);
                    _manager.PushOneEvaluates(nextApiImageIPtrBuffer, nextSize);
                    PRINT_COSTS(PushOneEvaluates);
                }
                else
                {
                    if (_manager._keypointParam.threadCount > 0)
                    {
                        START_EVALUATE(PushOneDetectKeypoints);
                        _manager.PushOneDetectKeypoints(nextApiImageIPtrBuffer, nextSize);
                        PRINT_COSTS(PushOneDetectKeypoints);
                    }
                }
            }
        }

        PRINT_FUNCTION_COSTS();
    }
}

bool FaceDetector::Detector::DetectFacesByResolutionGroup(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& trackApiImageIPtrBuffer, size_t& trackSize, ApiImagePtrBuffer& nextApiImageIPtrBuffer, size_t& nextSize)
{
    DECLARE_FPS_STATIC(5000);

    ResolutionGroup resolutionGroup;
    // detect faces by resolution
    size_t batchSize = apiImageIPtrBatch.size();
    for (size_t idx = 0; idx < batchSize; ++idx)
    {
        ApiImagePtr& apiImagePtr = apiImageIPtrBatch[idx];
        int resolution = apiImagePtr->ResolutionType();
        ResolutionGroup::iterator groupItr = resolutionGroup.find(resolution);
        if (groupItr == resolutionGroup.end())
        {
            ResolutionIndex indexs;
            indexs.push_back((int)idx);
            resolutionGroup.insert(std::make_pair(resolution, indexs));
        }
        else
        {
            groupItr->second.push_back((int)idx);
        }
    }

    for (ResolutionGroup::iterator groupItr = resolutionGroup.begin(); groupItr != resolutionGroup.end(); ++groupItr)
    {
        ApiImagePtrBatch detectedApiImagePtrs;
        FaceSdkImages detectedfaceSdkImages;
        for each(int index in groupItr->second)
        {
            ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[index];

            detectedfaceSdkImages.push_back(apiImageIPtr->sdkImage);
            detectedApiImagePtrs.push_back(apiImageIPtr);
        }

        MultiFaceSdkBoxes multiFaceSdkBoxes;
        START_EVALUATE(DetectFaces);
        if (detectedfaceSdkImages.size() <= 0 || DetectFaces(_channel, detectedfaceSdkImages, multiFaceSdkBoxes, _manager._detectParam.faceThreshold) != FaceSdkOk)
        {
            continue;
        }
        PRINT_COSTS(DetectFaces);

        for (size_t detectIndex = 0; detectIndex < detectedApiImagePtrs.size(); ++detectIndex)
        {
            if (multiFaceSdkBoxes[detectIndex].size() > 0)
            {
                ApiImagePtr& detectedApiImageIPtr = detectedApiImagePtrs[detectIndex];
                detectedApiImageIPtr->sdkBoxes.swap(multiFaceSdkBoxes[detectIndex]);
                
                // portrait does not have track id, so we have to change it to which the user provided
                if (detectedApiImageIPtr->portrait)
                {
                    detectedApiImageIPtr->UpdatePortraitTrackId();
                    nextApiImageIPtrBuffer.push_back(detectedApiImageIPtr);
                    nextSize++;
                }
                else
                {
                    trackApiImageIPtrBuffer.push_back(detectedApiImageIPtr);
                    trackSize++;
                }
            }
        }

        PRINT("%s detect image number: %d\n", __FUNCTION__, detectedfaceSdkImages.size());
    }
    
    STATISTIC_FPS(batchSize, __FUNCTION__);

    return nextSize > 0 || trackSize > 0;
}


FaceDetector::Tracker::Tracker(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableTrack = true;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::Tracker::~Tracker()
{

}

void FaceDetector::Tracker::Work()
{
    ApiImagePtrBatch apiImageIPtrBatch;
    START_EVALUATE(FetchOneTrack);
    if (_manager.FetchOneTrack(apiImageIPtrBatch))
    {
        PRINT_COSTS(FetchOneTrack);

        DO_IF(PRINT("%s buffer size: %d, buffered image size: %d\n", __FUNCTION__, _manager._trackBuffer.size(), _manager._trackImageNumber), apiImageIPtrBatch.size() <= 2);

        START_FUNCTION_EVALUATE();

        ApiImagePtrBuffer trackedApiImageIPtrBuffer;
        size_t trackedSize = 0;
        START_EVALUATE(TrackOne);
        if (TrackOne(apiImageIPtrBatch, trackedApiImageIPtrBuffer, trackedSize))
        {
            PRINT_COSTS(TrackOne);

            if (trackedSize > 0)
            {
                if (_manager._evaluateParam.threadCount > 0)
                {
                    START_EVALUATE(PushOneEvaluates);
                    _manager.PushOneEvaluates(trackedApiImageIPtrBuffer, trackedSize);
                    PRINT_COSTS(PushOneEvaluates);
                }
                else
                {
                    if (_manager._keypointParam.threadCount > 0)
                    {
                        START_EVALUATE(PushOneDetectKeypoints);
                        _manager.PushOneDetectKeypoints(trackedApiImageIPtrBuffer, trackedSize);
                        PRINT_COSTS(PushOneDetectKeypoints);
                    }
                }
            }
        }

        PRINT_FUNCTION_COSTS();
    }
}

bool FaceDetector::Tracker::TrackOne(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& trackedApiImageIPtrBuffer, size_t& trackedSize)
{
    DECLARE_FPS_STATIC(5000);

    size_t trackSize = apiImageIPtrBatch.size();
    FaceSdkImages faceSdkImages(trackSize);
    SourceIds sourceIds(trackSize);
    MultiFaceSdkBoxes multiFaceSdkBoxes(trackSize);
    for (size_t idxBefore = 0; idxBefore < trackSize; ++idxBefore)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxBefore];
        //apiImageIPtr->Show();

        TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, apiImageIPtr->sourceId, apiImageIPtr->imageId, TimeStamp<MILLISECONDS>::Now());

        faceSdkImages[idxBefore] = apiImageIPtr->sdkImage;
        sourceIds[idxBefore] = apiImageIPtr->sourceId;
        multiFaceSdkBoxes[idxBefore].swap(apiImageIPtr->sdkBoxes);
    }
    START_EVALUATE(Track);
    if (Track(_channel, faceSdkImages, multiFaceSdkBoxes, _manager._detectParam.faceThreshold, sourceIds) != FaceSdkOk)
    {
        return false;
    }
    PRINT_COSTS(Track);

    STATISTIC_FPS(trackSize, __FUNCTION__);

    PRINT("%s track image number: %d\n", __FUNCTION__, trackSize);

    for (size_t idxAfter = 0; idxAfter < trackSize; ++idxAfter)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxAfter];
        const FaceParam& faceParam = apiImageIPtr->faceParam;
        FaceSdkBoxes& apiFaceSdkBoxes = apiImageIPtr->sdkBoxes;

        FaceSdkBoxes& trackedFaceSdkBoxes = multiFaceSdkBoxes[idxAfter];

        //apiImageIPtr->Show();

        PRINT("%s face number: %d\n", __FUNCTION__, trackedFaceSdkBoxes.size());

        // check if has face
        if (trackedFaceSdkBoxes.size() > 0)
        {
            for each (FaceSdkBox faceSdkBox in trackedFaceSdkBoxes)
            {
                PRINT("%s face(%d, %d, %d, %d)\n", __FUNCTION__,
                    faceSdkBox.x, faceSdkBox.y, faceSdkBox.width, faceSdkBox.height);

                if (faceSdkBox.width >= faceParam.min_face_size && faceSdkBox.height >= faceParam.min_face_size
                    && faceSdkBox.width <= faceParam.max_face_size && faceSdkBox.height <= faceParam.max_face_size)
                {
                    apiFaceSdkBoxes.push_back(faceSdkBox);
                }
            }

            if (apiFaceSdkBoxes.size() > 0)
            {
                trackedApiImageIPtrBuffer.push_back(apiImageIPtr);
                trackedSize++;
            }
        }
    }

    return trackedSize > 0;
}

FaceDetector::Evaluator::Evaluator(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableEvaluate = true;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::Evaluator::~Evaluator()
{

}

void FaceDetector::Evaluator::Work()
{
    ApiImagePtrBatch apiImageIPtrBatch;
    START_EVALUATE(FetchOneEvaluates);
    if (_manager.FetchOneEvaluates(apiImageIPtrBatch))
    {
        PRINT_COSTS(FetchOneEvaluates);

        START_FUNCTION_EVALUATE();

        ApiImagePtrBuffer filteredApiImageIPtrBuffer;
        size_t filteredSize = 0;
        START_EVALUATE(EvaluateOneBadness);
        if (EvaluateOneBadness(apiImageIPtrBatch, filteredApiImageIPtrBuffer, filteredSize))
        {
            PRINT_COSTS(EvaluateOneBadness);

            if (_manager._keypointParam.threadCount > 0)
            {
                START_EVALUATE(PushOneDetectKeypoints);
                _manager.PushOneDetectKeypoints(filteredApiImageIPtrBuffer, filteredSize);
                PRINT_COSTS(PushOneDetectKeypoints);
            }
        }

        PRINT_FUNCTION_COSTS();
    }
}

bool FaceDetector::Evaluator::FilterFaces(ApiImagePtrBatch& apiImageIPtrBatch, MultiFaceSdkBoxes& multiFaceSdkBoxes, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize)
{
    START_FUNCTION_EVALUATE();

    size_t batchSize = apiImageIPtrBatch.size();
    for (size_t idx = 0; idx < batchSize; ++idx)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idx];
        const FaceParam& faceParam = apiImageIPtr->faceParam;
        FaceSdkBoxes& apiFaceSdkBoxes = apiImageIPtr->sdkBoxes;

        FaceSdkBoxes& badnessFaceSdkBoxes = multiFaceSdkBoxes[idx];

        for each(FaceSdkBox badnessFaceSdkBox in badnessFaceSdkBoxes)
        {
            PRINT("%s face(%d,%d,%d,%d) badness: %.06f, target badness: %.06f\n", __FUNCTION__,
                badnessFaceSdkBox.x, badnessFaceSdkBox.y, badnessFaceSdkBox.width, badnessFaceSdkBox.height,
                badnessFaceSdkBox.badness, faceParam.badness);

            if (faceParam.badness < 0.0f || badnessFaceSdkBox.badness < faceParam.badness)
            {
                apiFaceSdkBoxes.push_back(badnessFaceSdkBox);
            }
        }

        if (apiFaceSdkBoxes.size() > 0)
        {
            filteredApiImageIPtrBuffer.push_back(apiImageIPtr);
            filteredSize++;
        }
    }

    PRINT_FUNCTION_COSTS();

    return filteredSize > 0;
}

bool FaceDetector::Evaluator::EvaluateOneBadness(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize)
{
    DECLARE_FPS_STATIC(5000);

    size_t batchSize = apiImageIPtrBatch.size();
    FaceSdkImages faceSdkImages(batchSize);
    MultiFaceSdkBoxes multiFaceSdkBoxes(batchSize);
    for (size_t idxBefore = 0; idxBefore < batchSize; ++idxBefore)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxBefore];

        TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, apiImageIPtr->sourceId, apiImageIPtr->imageId, TimeStamp<MILLISECONDS>::Now());

        faceSdkImages[idxBefore] = apiImageIPtr->sdkImage;
        multiFaceSdkBoxes[idxBefore].swap(apiImageIPtr->sdkBoxes);
    }

    START_EVALUATE(EvaluateBadness);
    if (EvaluateBadness(_channel, faceSdkImages, multiFaceSdkBoxes) != FaceSdkOk)
    {
        return false;
    }
    PRINT_COSTS(EvaluateBadness);

    STATISTIC_FPS(batchSize, __FUNCTION__);

    PRINT("%s evaluate image number: %d\n", __FUNCTION__, batchSize);

    return FilterFaces(apiImageIPtrBatch, multiFaceSdkBoxes, filteredApiImageIPtrBuffer, filteredSize);
}

FaceDetector::KeyPointer::KeyPointer(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableDetectKeypoint = true;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::KeyPointer::~KeyPointer()
{

}

void FaceDetector::KeyPointer::Work()
{
    ApiImagePtrBatch apiImageIPtrBatch;
    START_EVALUATE(FetchOneDetectKeypoints);
    if (_manager.FetchOneDetectKeypoints(apiImageIPtrBatch))
    {
        PRINT_COSTS(FetchOneDetectKeypoints);

        START_FUNCTION_EVALUATE();

        ApiImagePtrBuffer filteredApiImageIPtrBuffer;
        size_t filteredSize = 0;
        START_EVALUATE(DeteckKeypointsOne);
        if (DeteckKeypointsOne(apiImageIPtrBatch, filteredApiImageIPtrBuffer, filteredSize))
        {
            PRINT_COSTS(DeteckKeypointsOne);

            START_EVALUATE(PushOneAlign);
            _manager.PushOneAlign(filteredApiImageIPtrBuffer, filteredSize);
            PRINT_COSTS(PushOneAlign);
        }

        PRINT_FUNCTION_COSTS();
    }
}

bool FaceDetector::KeyPointer::FilterFaces(ApiImagePtrBatch& apiImageIPtrBatch, MultiFaceSdkBoxes& multiFaceSdkBoxes, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize)
{
    START_FUNCTION_EVALUATE();

    size_t batchSize = apiImageIPtrBatch.size();
    FaceStatFinder& faceStatFinder = _manager._faceStatFinder;
    for (size_t idx = 0; idx < batchSize; ++idx)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idx];
        const FaceParam& faceParam = apiImageIPtr->faceParam;
        const SourceId& sourceId = apiImageIPtr->sourceId;
        FaceSdkBoxes& apiFaceSdkBoxes = apiImageIPtr->sdkBoxes;

        FaceSdkBoxes& keypointedFaceSdkBoxes = multiFaceSdkBoxes[idx];

        for each(FaceSdkBox keypointedFaceSdkBox in keypointedFaceSdkBoxes)
        {
            PRINT("%s confidence: %f, keypoint confidence: %f, pitch: %f, yaw: %f, roll: %f\n", __FUNCTION__, 
                keypointedFaceSdkBox.confidence, keypointedFaceSdkBox.keypointsConfidence,
                keypointedFaceSdkBox.angles[ANGLE_PITCH], keypointedFaceSdkBox.angles[ANGLE_YAW], keypointedFaceSdkBox.angles[ANGLE_ROLL]);

            if ((faceParam.keypointsConfidence < FLOAT_CONFIGURED || keypointedFaceSdkBox.keypointsConfidence >= faceParam.keypointsConfidence)
                &&
                (faceParam.angle_pitch < FLOAT_CONFIGURED || (keypointedFaceSdkBox.angles[ANGLE_PITCH] >= -faceParam.angle_pitch && keypointedFaceSdkBox.angles[ANGLE_PITCH] <= faceParam.angle_pitch))
                &&
                (faceParam.angle_yaw < FLOAT_CONFIGURED || (keypointedFaceSdkBox.angles[ANGLE_YAW] >= -faceParam.angle_yaw && keypointedFaceSdkBox.angles[ANGLE_YAW] <= faceParam.angle_yaw))
                &&
                (faceParam.angle_roll < FLOAT_CONFIGURED || (keypointedFaceSdkBox.angles[ANGLE_ROLL] >= -faceParam.angle_roll && keypointedFaceSdkBox.angles[ANGLE_ROLL] <= faceParam.angle_roll)))
            {
                if (faceParam.capture_interval > 0)
                {
                    FaceStat faceStat;
                    if (faceStatFinder.Find(keypointedFaceSdkBox.number, faceStat, sourceId))
                    {
                        if (faceStat.trackedFrameNumber < faceParam.capture_frame_number)
                        {
                            apiFaceSdkBoxes.push_back(keypointedFaceSdkBox);

                            ++faceStat.trackedFrameNumber;
                            faceStatFinder.Update(keypointedFaceSdkBox.number, faceStat, sourceId);
                        }
                    }
                    else
                    {
                        // should larger than 0, or no frame will output
                        if (faceParam.capture_frame_number > 0)
                        {
                            faceStat.trackId = keypointedFaceSdkBox.number;
                            faceStat.trackedFrameNumber = 1;
                            faceStatFinder.Add(keypointedFaceSdkBox.number, faceStat, faceParam.capture_interval, sourceId, 0, faceParam.capture_interval);

                            apiFaceSdkBoxes.push_back(keypointedFaceSdkBox);
                        }
                    }
                }
                else
                {
                    apiFaceSdkBoxes.push_back(keypointedFaceSdkBox);
                }
            }
        }

        if (apiFaceSdkBoxes.size() > 0)
        {
            filteredApiImageIPtrBuffer.push_back(apiImageIPtr);
            filteredSize++;
        }
    }

    PRINT_FUNCTION_COSTS();

    return filteredSize > 0;
}

bool FaceDetector::KeyPointer::DeteckKeypointsOne(ApiImagePtrBatch& apiImageIPtrBatch, ApiImagePtrBuffer& filteredApiImageIPtrBuffer, size_t& filteredSize)
{
    DECLARE_FPS_STATIC(5000);

    size_t batchSize = apiImageIPtrBatch.size();
    FaceSdkImages faceSdkImages(batchSize);
    MultiFaceSdkBoxes multiFaceSdkBoxes(batchSize);
    for (size_t idxBefore = 0; idxBefore < batchSize; ++idxBefore)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxBefore];

        //apiImageIPtr->Save("E:/tests/ta" + apiImageIPtr->sourceId + "/" + std::to_string(clock()) + ".jpg");

        TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, apiImageIPtr->sourceId, apiImageIPtr->imageId, TimeStamp<MILLISECONDS>::Now());

        faceSdkImages[idxBefore] = apiImageIPtr->sdkImage;
        multiFaceSdkBoxes[idxBefore].swap(apiImageIPtr->sdkBoxes);
    }

    START_EVALUATE(DetectKeypoints);
    if (DetectKeypoints(_channel, faceSdkImages, multiFaceSdkBoxes) != FaceSdkOk)
    {
        return false;
    }
    PRINT_COSTS(DetectKeypoints);

    STATISTIC_FPS(batchSize, __FUNCTION__);

    PRINT("%s key pointer image number: %d\n", __FUNCTION__, batchSize);
    
    return FilterFaces(apiImageIPtrBatch, multiFaceSdkBoxes, filteredApiImageIPtrBuffer, filteredSize);
}

CaptureResultPtr FaceDetector::Aligner::GenerateCaptureResult(ApiImagePtr& apiImagePtr, const FaceBox& faceBox)
{
    CaptureResultPtr captureResultPtr(new CaptureResult());
    if (captureResultPtr)
    {
        // keep scence image
        apiImagePtr->KeepScence(*_stream);

        captureResultPtr->frameId = apiImagePtr->imageId;
        captureResultPtr->sourceId = apiImagePtr->sourceId;
        captureResultPtr->timestamp = apiImagePtr->timestamp;
        captureResultPtr->position = apiImagePtr->position;
        captureResultPtr->faceBox = faceBox;

        // copy scence image and face image
        captureResultPtr->scence = apiImagePtr->scence;
        apiImagePtr->KeepFace(*_stream, captureResultPtr->face, faceBox.x, faceBox.y, faceBox.width, faceBox.height);
    }
    return captureResultPtr;
}

FaceDetector::Aligner::Aligner(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam), _stream(nullptr)
{
    _channelParam.enableAlign = true;
    if (_gpuIndex >= 0)
    {
        _stream = new cv::cuda::Stream();
    }
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::Aligner::~Aligner()
{

}

void FaceDetector::Aligner::Work()
{
    ApiImagePtrBatch apiImageIPtrBatch;
    START_EVALUATE(FetchOneAlign);
    if (_manager.FetchOneAlign(apiImageIPtrBatch))
    {
        PRINT_COSTS(FetchOneAlign);

        START_FUNCTION_EVALUATE();

        AnalyzeResultPtrBuffer toExtractFeatureAnalyzeResultPtrBuffer, toAnalyzeAttrAnalyzeResultPtrBuffer;
        size_t toExtractFeatureAnalyzeResultPtrBufferSize = 0, toAnalyzeAttrAnalyzeResultPtrBufferSize = 0;
        CaptureResults captureResults;
        START_EVALUATE(AlignOne);
        if (AlignOne(apiImageIPtrBatch, toExtractFeatureAnalyzeResultPtrBuffer, toExtractFeatureAnalyzeResultPtrBufferSize, 
            toAnalyzeAttrAnalyzeResultPtrBuffer, toAnalyzeAttrAnalyzeResultPtrBufferSize, captureResults))
        {
            PRINT_COSTS(AlignOne);

            if (toExtractFeatureAnalyzeResultPtrBufferSize > 0)
            {
                START_EVALUATE(PushOneExtract);
                _manager.PushOneExtract(toExtractFeatureAnalyzeResultPtrBuffer, toExtractFeatureAnalyzeResultPtrBufferSize);
                PRINT_COSTS(PushOneExtract);
            }

            if (toAnalyzeAttrAnalyzeResultPtrBufferSize > 0)
            {
                START_EVALUATE(PushOneAnalyze);
                _manager.PushOneAnalyze(toAnalyzeAttrAnalyzeResultPtrBuffer, toAnalyzeAttrAnalyzeResultPtrBufferSize);
                PRINT_COSTS(PushOneAnalyze);
            }

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

bool FaceDetector::Aligner::AlignOne(ApiImagePtrBatch& apiImageIPtrBatch, AnalyzeResultPtrBuffer& toExtractResultPtrBuffer, size_t& toExtractResultPtrBufferSize,
    AnalyzeResultPtrBuffer& toAnalyzeResultPtrBuffer, size_t& toAnalyzeResultPtrBufferSize, CaptureResults& captureResults)
{
    DECLARE_FPS_STATIC(5000);

    size_t batchSize = apiImageIPtrBatch.size();
    FaceSdkImages faceSdkImages(batchSize);
    MultiFaceSdkBoxes multiFaceSdkBoxes(batchSize);
    for (size_t idxAlign = 0; idxAlign < batchSize; ++idxAlign)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxAlign];
        TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, apiImageIPtr->sourceId, apiImageIPtr->imageId, TimeStamp<MILLISECONDS>::Now());

        faceSdkImages[idxAlign] = apiImageIPtr->sdkImage;
        multiFaceSdkBoxes[idxAlign].swap(apiImageIPtr->sdkBoxes);
    }

    START_EVALUATE(Align);
    if (faceSdkImages.size() == 0 || Align(_channel, faceSdkImages, multiFaceSdkBoxes) != FaceSdkOk)
    {
        return false;
    }
    PRINT_COSTS(Align);

    PRINT("%s align image number: %d\n", __FUNCTION__, batchSize);

    STATISTIC_FPS(batchSize, __FUNCTION__);

    int devIndex = _manager._alignParam.deviceIndex;

    const AnalyzeParam& analyzeParam = _manager._analyzeParam;
    FaceAttriFinder& faceAttriFinder = _manager._faceAttriFinder;
    BestFaceFinder& bestFaceFinder = _manager._bestFaceFinder;

    bool hasExtractor = _manager._faceExtractor != nullptr;

    START_EVALUATE(DownloadScenceAndFaceImage);
    for (size_t idxAfter = 0; idxAfter < batchSize; ++idxAfter)
    {
        ApiImagePtr& apiImageIPtr = apiImageIPtrBatch[idxAfter];
        FaceSdkBoxes& afterSdkBoxes = multiFaceSdkBoxes[idxAfter];

        // process each face
        if (afterSdkBoxes.size() > 0)
        {
            // alias
            const FaceParam& faceParam = apiImageIPtr->faceParam;
            const std::string& sourceId = apiImageIPtr->sourceId;

            // iterate each face
            for (size_t idxSdkBoxCheck = 0; idxSdkBoxCheck < afterSdkBoxes.size(); ++idxSdkBoxCheck)
            {
                FaceSdkBox& afterSdkBox = afterSdkBoxes[idxSdkBoxCheck];

                FaceBox faceBox;
                // get the face box information
                ApiImage::ToFaceBox(faceBox, afterSdkBox);

                PRINT("%s face info: %d(%d,%d,%d,%d)\n", __FUNCTION__, faceBox.id, faceBox.x, faceBox.y, faceBox.width, faceBox.height);
                
                // need to analyze face attribute
                if (analyzeParam.threadCount > 0
                    && ((faceParam.analyze_mask && analyzeParam.analyze_mask)
                    || (faceParam.analyze_glasses && analyzeParam.analyze_glasses)
                    || (faceParam.analyze_age_ethnic && analyzeParam.analyze_age_ethnic)
                    || (faceParam.analyze_age_gender && analyzeParam.analyze_age_gender)
                    || (faceParam.clarity > 0.0f && analyzeParam.analyze_clarity)
                    || (faceParam.brightness > 0.0f && analyzeParam.analyze_brightness))
                    )
                {
                    FaceBox oldFaceBox;
                    if (faceAttriFinder.Find(faceBox.id, oldFaceBox, sourceId))
                    {
                        faceBox.mask = oldFaceBox.mask;
                        faceBox.glasses = oldFaceBox.glasses;
                        faceBox.age = oldFaceBox.age;
                        faceBox.gender = oldFaceBox.gender;
                        faceBox.age_group = oldFaceBox.age_group;
                        faceBox.ethnic = oldFaceBox.ethnic;
                    }
                    else
                    {
                        toAnalyzeResultPtrBuffer.push_back(AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)));
                        toAnalyzeResultPtrBufferSize++;
                        continue;
                    }
                }

                if (faceParam.choose_best_interval > 0)
                {
                    AnalyzeResultPtr betterAnalyzeResultPtr(nullptr);
                    if (bestFaceFinder.Find(faceBox.id, betterAnalyzeResultPtr, sourceId))
                    {
                        if (betterAnalyzeResultPtr)
                        {
                            if (betterAnalyzeResultPtr->captureResultPtr->faceBox.keypointsConfidence < faceBox.keypointsConfidence)
                            {
                                bestFaceFinder.Update(faceBox.id, AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)), sourceId);
                            }
                        }
                        else
                        {
                            bestFaceFinder.Update(faceBox.id, AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)), sourceId);
                        }
                    }
                    else
                    {
                        if (faceParam.choose_entry_timeout > 0)
                        {
                            bestFaceFinder.Add(faceBox.id, AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)), faceParam.choose_best_interval, sourceId, faceParam.choose_entry_timeout, faceParam.choose_best_interval);
                        }
                        else
                        {
                            if (faceParam.extract_feature && hasExtractor)
                            {
                                toExtractResultPtrBuffer.push_back(AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)));
                                toExtractResultPtrBufferSize++;
                            }
                            else
                            {
                                captureResults.push_back(GenerateCaptureResult(apiImageIPtr, faceBox));
                            }
                            bestFaceFinder.Add(faceBox.id, nullptr, faceParam.choose_best_interval, sourceId, 0, faceParam.choose_best_interval);
                        }
                    }
                }
                else
                {
                    if (faceParam.extract_feature && hasExtractor)
                    {
                        toExtractResultPtrBuffer.push_back(AnalyzeResultPtr(new AnalyzeResult(devIndex, GenerateCaptureResult(apiImageIPtr, faceBox), afterSdkBox.aligned, faceParam)));
                        toExtractResultPtrBufferSize++;
                    }
                    else
                    {
                        captureResults.push_back(GenerateCaptureResult(apiImageIPtr, faceBox));
                    }
                }                    
            }
        }
    }
    PRINT_COSTS(DownloadScenceAndFaceImage);

    return captureResults.size() > 0 || toExtractResultPtrBufferSize > 0 || toAnalyzeResultPtrBufferSize > 0;
}

FaceDetector::FaceAttrAnalyzer::FaceAttrAnalyzer(FaceDetector& manager, int gpuIndex, FaceSdkParam& channelParam)
    : Worker(manager, gpuIndex, channelParam)
{
    _channelParam.enableEvaluate = _manager._analyzeParam.analyze_clarity || _manager._analyzeParam.analyze_brightness;
    _channelParam.enableAnalyzeAgeGender = _manager._analyzeParam.analyze_age_gender;
    _channelParam.enableAnalyzeAgeEthnic = _manager._analyzeParam.analyze_age_ethnic;
    _channelParam.enableAnalyzeGlasses = _manager._analyzeParam.analyze_glasses;
    _channelParam.enableAnalyzeMask = _manager._analyzeParam.analyze_mask;
    strncpy(_name, __FUNCTION__, sizeof(_name) - 1);
}

FaceDetector::FaceAttrAnalyzer::~FaceAttrAnalyzer()
{
}

void FaceDetector::FaceAttrAnalyzer::PreHeat()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void FaceDetector::FaceAttrAnalyzer::Work()
{
    AnalyzeResultPtrBatch analyzedResultPtrs;
    START_EVALUATE(FetchOneAnalyze);
    if (_manager.FetchOneAnalyze(analyzedResultPtrs))
    {
        PRINT_COSTS(FetchOneAnalyze);

        START_FUNCTION_EVALUATE();

        AnalyzeResultPtrBuffer toExtractResultPtrBuffer;
        size_t toExtractResultPtrBufferSize = 0;
        CaptureResults captureResults;
        START_EVALUATE(AnalyzeOne);
        AnalyzeOne(analyzedResultPtrs, toExtractResultPtrBuffer, toExtractResultPtrBufferSize, captureResults);
        PRINT_COSTS(AnalyzeOne);

        if (toExtractResultPtrBufferSize > 0)
        {
            START_EVALUATE(PushOneExtract);
            _manager.PushOneExtract(toExtractResultPtrBuffer, toExtractResultPtrBufferSize);
            PRINT_COSTS(PushOneExtract);
        }

        if (captureResults.size() > 0)
        {
            START_EVALUATE(PushOneResults);
            _manager.PushOneResults(captureResults);
            PRINT_COSTS(PushOneResults);
        }

        PRINT_FUNCTION_COSTS();
    }
}

void FaceDetector::FaceAttrAnalyzer::AnalyzeOne(AnalyzeResultPtrBatch& analyzedResultPtrs, AnalyzeResultPtrBuffer& toExtractResultPtrBuffer, size_t& toExtractResultPtrBufferSize, CaptureResults& captureResults)
{
    DECLARE_FPS_STATIC(5000);

    bool analyze_clarity = _manager._analyzeParam.analyze_clarity;
    bool analyze_brightness = _manager._analyzeParam.analyze_brightness;
    int devIndex = _manager._analyzeParam.deviceIndex;

    size_t batchSize = analyzedResultPtrs.size();
    FaceSdkImages ageGenderAnalyzeSdkImages, ageEthicAnalyzeSdkImages, claritySdkImages;
    
    START_EVALUATE(BrightMaskGlass);
    for (size_t idxBefore = 0; idxBefore <batchSize; ++idxBefore)
    {
        AnalyzeResultPtr& analyzedResultPtr = analyzedResultPtrs[idxBefore];

        if (analyzedResultPtr->CreateSdkImage(devIndex))
        {
            FaceBox& faceBox = analyzedResultPtr->captureResultPtr->faceBox;
            const FaceParam& faceParam = analyzedResultPtr->faceParam;

            if (_channelParam.enableAnalyzeAgeGender && faceParam.analyze_age_gender)
            {
                ageGenderAnalyzeSdkImages.push_back(analyzedResultPtr->alignedImage);
            }
            if (_channelParam.enableAnalyzeAgeEthnic && faceParam.analyze_age_ethnic)
            {
                ageEthicAnalyzeSdkImages.push_back(analyzedResultPtr->alignedImage);
            }
            if (analyze_clarity && faceParam.clarity > 0.0f)
            {
                claritySdkImages.push_back(analyzedResultPtr->alignedImage);
            }

            if (_channelParam.enableAnalyzeGlasses && faceParam.analyze_glasses)
            {
                bool glasses = false;
                // analyze glass state
                START_EVALUATE(AnalyzeGlasses);
                AnalyzeGlasses(_channel, analyzedResultPtr->alignedImage, glasses);
                PRINT_COSTS(AnalyzeGlasses);
                faceBox.glasses = glasses ? 1 : 0;
            }

            if (_channelParam.enableAnalyzeMask && faceParam.analyze_mask)
            {
                bool mask = false;
                // analyze mask state
                START_EVALUATE(AnalyzeMask);
                AnalyzeMask(_channel, analyzedResultPtr->alignedImage, mask);
                PRINT_COSTS(AnalyzeMask);
                faceBox.mask = mask ? 1 : 0;
            }

            if (analyze_brightness && faceParam.brightness > 0.0f)
            {
                // analyze bright
                START_EVALUATE(EvaluateBrightness);
                EvaluateBrightness(_channel, analyzedResultPtr->alignedImage, faceBox.brightness);
                PRINT_COSTS(EvaluateBrightness);
            }
        }
    }
    PRINT_COSTS(BrightMaskGlass);

    START_EVALUATE(AnalyzeAgeEthnics);
    if (ageEthicAnalyzeSdkImages.size() > 0)
    {
        AgeGroups ageGroups;
        Ethnics ethnics;
        START_EVALUATE(AnalyzeAgeEthnic);
        FaceSdkResult sdkResult = AnalyzeAgeEthnic(_channel, ageEthicAnalyzeSdkImages, ageGroups, ethnics);
        PRINT_COSTS(AnalyzeAgeEthnic);

        if (sdkResult == FaceSdkOk)
        {
            size_t ageIndex = 0;
            for (size_t idxAfter = 0; idxAfter < analyzedResultPtrs.size(); ++idxAfter)
            {
                AnalyzeResultPtr& analyzedResultPtr = analyzedResultPtrs[idxAfter];
                FaceBox& faceBox = analyzedResultPtr->captureResultPtr->faceBox;
                if (analyzedResultPtr->alignedImage)
                {
                    const FaceParam faceParam = analyzedResultPtr->faceParam;

                    if (_channelParam.enableAnalyzeAgeEthnic && faceParam.analyze_age_ethnic)
                    {
                        faceBox.age_group = ageGroups[ageIndex];
                        faceBox.ethnic = ethnics[ageIndex++];
                    }
                }
            }
        }
    }
    PRINT_COSTS(AnalyzeAgeEthnics);

    START_EVALUATE(AnalyzeAgeGenders);
    if (ageGenderAnalyzeSdkImages.size() > 0)
    {
        Ages ages;
        Genders genders;
        START_EVALUATE(AnalyzeAgeGender);
        FaceSdkResult sdkResult = AnalyzeAgeGender(_channel, ageGenderAnalyzeSdkImages, ages, genders);
        PRINT_COSTS(AnalyzeAgeGender);
        
        if (sdkResult == FaceSdkOk)
        {
            size_t ageIndex = 0;
            for (size_t idxAfter = 0; idxAfter < analyzedResultPtrs.size(); ++idxAfter)
            {
                AnalyzeResultPtr& analyzedResultPtr = analyzedResultPtrs[idxAfter];
                FaceBox& faceBox = analyzedResultPtr->captureResultPtr->faceBox;
                if (analyzedResultPtr->alignedImage)
                {
                    const FaceParam faceParam = analyzedResultPtr->faceParam;

                    if (_channelParam.enableAnalyzeAgeGender && faceParam.analyze_age_gender)
                    {
                        faceBox.age = ages[ageIndex];
                        faceBox.gender = genders[ageIndex++];
                    }
                }
            }
        }
    }
    PRINT_COSTS(AnalyzeAgeGenders);

    START_EVALUATE(EvaluateClaritys);
    if (claritySdkImages.size() > 0)
    {
        std::vector<float> clarities;
        START_EVALUATE(EvaluateClarity);
        FaceSdkResult sdkResult = EvaluateClarity(_channel, claritySdkImages, clarities);
        PRINT_COSTS(EvaluateClarity);

        if (sdkResult == FaceSdkOk)
        {
            size_t clarityIndex = 0;
            for (size_t idxAfter = 0; idxAfter < analyzedResultPtrs.size(); ++idxAfter)
            {
                AnalyzeResultPtr& analyzedResultPtr = analyzedResultPtrs[idxAfter];
                FaceBox& faceBox = analyzedResultPtr->captureResultPtr->faceBox;
                if (analyzedResultPtr->alignedImage)
                {
                    const FaceParam& faceParam = analyzedResultPtr->faceParam;

                    if (faceParam.clarity > 0.0f)
                    {
                        faceBox.clarity = clarities[clarityIndex++];
                    }
                }
            }
        }
    }
    PRINT_COSTS(EvaluateClaritys);

    START_EVALUATE(UpdateAnalyseResult);

    bool hasExtractor = _manager._faceExtractor != nullptr;
    FaceAttriFinder& faceAttriFinder = _manager._faceAttriFinder;
    BestFaceFinder& bestFaceFinder = _manager._bestFaceFinder;
    // construct capture result
    for each(AnalyzeResultPtr analyzeResultPtr in analyzedResultPtrs)
    {
        const CaptureResultPtr& captureResultPtr = analyzeResultPtr->captureResultPtr;
        const FaceParam& faceParam = analyzeResultPtr->faceParam;
        const SourceId& sourceId = captureResultPtr->sourceId;
        FaceBox& faceBox = captureResultPtr->faceBox;

        if (faceParam.choose_best_interval > 0)
        {
            AnalyzeResultPtr betterAnalyzeResultPtr(nullptr);
            if (bestFaceFinder.Find(faceBox.id, betterAnalyzeResultPtr, sourceId))
            {
                if (betterAnalyzeResultPtr)
                {
                    if (betterAnalyzeResultPtr->captureResultPtr->faceBox.keypointsConfidence < faceBox.keypointsConfidence)
                    {
                        bestFaceFinder.Update(faceBox.id, analyzeResultPtr, sourceId);
                    }
                }
                else
                {
                    bestFaceFinder.Update(faceBox.id, analyzeResultPtr, sourceId);
                }
            }
            else
            {
                if (faceParam.choose_entry_timeout > 0)
                {
                    bestFaceFinder.Add(faceBox.id, analyzeResultPtr, faceParam.choose_best_interval, sourceId, faceParam.choose_entry_timeout, faceParam.choose_best_interval);
                }
                else
                {
                    if (faceParam.extract_feature && hasExtractor)
                    {
                        toExtractResultPtrBuffer.push_back(analyzeResultPtr);
                        toExtractResultPtrBufferSize++;
                    }
                    else
                    {
                        captureResults.push_back(captureResultPtr);
                    }
                    bestFaceFinder.Add(faceBox.id, nullptr, faceParam.choose_best_interval, sourceId, 0, faceParam.choose_best_interval);
                }
            }
        }
        else
        {
            if (faceParam.extract_feature && hasExtractor)
            {
                toExtractResultPtrBuffer.push_back(analyzeResultPtr);
                toExtractResultPtrBufferSize++;
            }
            else
            {
                captureResults.push_back(captureResultPtr);
            }
        }

        // update face attribute
        faceAttriFinder.Add(faceBox.id, faceBox, 0, sourceId, 0, faceParam.analyze_result_timeout);
    }
    PRINT_COSTS(UpdateAnalyseResult);

    STATISTIC_FPS(batchSize, __FUNCTION__);
}

void FaceDetector::BestFaceFinderCallback(void* context, AnalyzeResultPtrBuffer& analyzedResultPtrBuffer)
{
    if (context)
    {
        FaceDetector* faceDetector = (FaceDetector*)context;
        faceDetector->PushBests(analyzedResultPtrBuffer);
    }
}

FaceDetector::FaceDetector(const ModelParam& modelParam, const DetectParam& detectParam, 
    const TrackParam& trackParam, const EvaluateParam& evaluateParam, const KeypointParam& keypointParam, 
    const AlignParam& alignParam, const AnalyzeParam& analyzerParam, const ResultParam& resultParam, FaceExtractor* faceExtractor)
    : _started(false), _error_code(0)
    , _contextLocker(), _baseDecoders()
    , _prepareDetectBuffer(), _preparingDetectBuffer()
    , _oneWorkerReady(), _oneWorkerReadyLocker()
    , _modelParam(modelParam), _resultParam(resultParam), _channelParam()
    , _faceExtractor(faceExtractor)
    , _detectParam(detectParam), _updateDetectBatchSizeDynamic(detectParam.batchSize <= 0), _detectors(), _detectBufferCondition(), _detectBufferLocker(), _detectBuffer(), _detectImageNumber(0)
    , _trackParam(trackParam), _updateTrackBatchSizeDynamic(trackParam.batchSize <= 0), _trackers(), _trackBufferCondition(), _trackBufferLocker(), _trackBuffer(), _trackImageNumber(0)
    , _evaluateParam(evaluateParam), _updateEvaluateBatchSizeDynamic(evaluateParam.batchSize <= 0), _evaluators(), _evaluateBufferCondition(), _evaluateBufferLocker(), _evaluateBuffer(), _evaluateImageNumber(0)
    , _keypointParam(keypointParam), _updateKeypointBatchSizeDynamic(keypointParam.batchSize <= 0), _keypointers(), _keyPointsBufferCondition(), _keyPointsBufferLocker(), _keyPointsBuffer(), _keypointsImageNumber(0)
    , _alignParam(alignParam), _updateAlignBatchSizeDynamic(alignParam.batchSize <= 0), _aligners(), _alignBufferCondition(), _alignBufferLocker(), _alignBuffer(), _alignImageNumber(0)
    , _analyzeParam(analyzerParam), _faceAttrAnalyzerPtrs(), _faceAttrAnalyzeBufferCondition(), _faceAttrAnalyzeBufferLocker(), _faceAttrAnalyzeBuffer(), _faceNumberToAnalyze(0)
    , _outputBufferLocker(), _outputBuffer(), _resultFaceNumber(0)
    , _faceStatFinder(10, nullptr, nullptr, this, ResetFaceStat)
    , _faceAttriFinder(10, nullptr, nullptr, this, nullptr)
    , _bestFaceFinder(10, nullptr, BestFaceFinderCallback, this, ResetAnalyzeResultPtr)
{
    _channelParam.modelDir = _modelParam.path;
    _channelParam.featureModel = _modelParam.name;

    _channelParam.faceModel = _detectParam.faceModel;
    _channelParam.thresholdDF = _detectParam.threshold;

    _channelParam.maxShortEdge = _detectParam.maxShortEdge;

    _channelParam.thresholdT = _trackParam.threshold;

    // initialize to one
    if (_updateDetectBatchSizeDynamic)
    {
        _detectParam.batchSize = 1;
    }
    if (_updateTrackBatchSizeDynamic)
    {
        _trackParam.batchSize = 1;
    }
    if (_updateEvaluateBatchSizeDynamic)
    {
        _evaluateParam.batchSize = 1;
    }
    if (_updateKeypointBatchSizeDynamic)
    {
        _keypointParam.batchSize = 1;
    }
    if (_updateAlignBatchSizeDynamic)
    {
        _alignParam.batchSize = 1;
    }
}

FaceDetector::~FaceDetector()
{
}

void FaceDetector::Start() throw(BaseException)
{
    if (!_started)
    {
        _started = true;

        try
        {
            StartOneDetector(_detectParam.deviceIndex);
            StartOneTracker(_trackParam.deviceIndex);
            StartOneBadnessEvalutor(_evaluateParam.deviceIndex);
            StartOneKeypointer(_keypointParam.deviceIndex);
            StartOneAligner(_alignParam.deviceIndex);
            StartOneAnalyzer(_analyzeParam.deviceIndex);
        }
        catch (BaseException& ex)
        {
            Stop();
            throw ex;
        }

        StartPrepareDetectBuffer();
    }
}

void FaceDetector::Stop()
{
    if (_started)
    {
        _started = false;

        StopPrepareDetectBuffer();

        StopAnalyzers();
        StopAligners();
        StopKeypointers();
        StopBadnessEvalutors();
        StopTrackers();
        StopDetectors();
    }
}

void FaceDetector::AddDecoder(BaseDecoder* baseDecoder, const FaceParam& faceParam)
{
    if (baseDecoder)
    {
        AUTOLOCK(_contextLocker);
        baseDecoder->SetFaceParam(faceParam);
        _baseDecoders.push_back(baseDecoder);

        int batchSize = _baseDecoders.size();
        if (_updateDetectBatchSizeDynamic)
        {
            _detectParam.batchSize = batchSize;
        }
        if (_updateTrackBatchSizeDynamic)
        {
            _trackParam.batchSize = batchSize;
        }
        if (_updateEvaluateBatchSizeDynamic)
        {
            _evaluateParam.batchSize = batchSize;
        }
        if (_updateKeypointBatchSizeDynamic)
        {
            _keypointParam.batchSize = batchSize;
        }
        if (_updateAlignBatchSizeDynamic)
        {
            _alignParam.batchSize = batchSize;
        }
    }
}

void FaceDetector::DeleteDecoder(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        AUTOLOCK(_contextLocker);
        for (size_t idx = 0; idx < _baseDecoders.size(); ++idx)
        {
            if (_baseDecoders[idx] == baseDecoder)
            {
                _baseDecoders.erase(_baseDecoders.begin() + idx);
                break;
            }
        }

        int batchSize = _baseDecoders.size();
        batchSize = batchSize == 0 ? 1 : batchSize;
        if (_updateDetectBatchSizeDynamic)
        {
            _detectParam.batchSize = batchSize;
        }
        if (_updateTrackBatchSizeDynamic)
        {
            _trackParam.batchSize = batchSize;
        }
        if (_updateEvaluateBatchSizeDynamic)
        {
            _evaluateParam.batchSize = batchSize;
        }
        if (_updateKeypointBatchSizeDynamic)
        {
            _keypointParam.batchSize = batchSize;
        }
        if (_updateAlignBatchSizeDynamic)
        {
            _alignParam.batchSize = batchSize;
        }
    }
}

void FaceDetector::DelFaceExtractor()
{
    _faceExtractor = nullptr;
}

bool FaceDetector::GetCapture(CaptureResults& captureResults)
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

int FaceDetector::GetDeviceIndex()
{
    if (_detectParam.deviceIndex >= 0)
    {
        return _detectParam.deviceIndex;
    }
    else
    {
        return -1;
    }
}

void FaceDetector::StartOneDetector(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _detectParam.threadCount; ++idx)
    {
        DetectorPtr detectorPtr(new Detector(*this, gpuIndex, _channelParam));
        if (detectorPtr)
        {
            detectorPtr->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);

            WaitNotify(__FUNCTION__);

            _detectors.push_back(detectorPtr);
        }
    }
}

void FaceDetector::StopDetectors()
{
    for each (DetectorPtr detectorPtr in _detectors)
    {
        detectorPtr->ReadyToStop();
        detectorPtr->Stop();
    }
    _detectors.clear();
}

void FaceDetector::StartOneTracker(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _trackParam.threadCount; ++idx)
    {
        TrackerPtr trackerPtr(new Tracker(*this, gpuIndex, _channelParam));
        if (trackerPtr)
        {
            trackerPtr->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);

            WaitNotify(__FUNCTION__);

            _trackers.push_back(trackerPtr);
        }
    }
}

void FaceDetector::StopTrackers()
{
    for each (TrackerPtr trackerPtr in _trackers)
    {
        trackerPtr->ReadyToStop();
        trackerPtr->Stop();
    }
    _trackers.clear();
}

void FaceDetector::StartOneBadnessEvalutor(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _evaluateParam.threadCount; ++idx)
    {
        EvaluatorPtr badnessEvaluator(new Evaluator(*this, gpuIndex, _channelParam));
        if (badnessEvaluator)
        {
            badnessEvaluator->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);

            WaitNotify(__FUNCTION__);

            _evaluators.push_back(badnessEvaluator);
        }
    }
}

void FaceDetector::StopBadnessEvalutors()
{
    for each (EvaluatorPtr badnessEvaluator in _evaluators)
    {
        badnessEvaluator->ReadyToStop();
        badnessEvaluator->Stop();
    }
    _evaluators.clear();
}

void FaceDetector::StartOneKeypointer(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _keypointParam.threadCount; ++idx)
    {
        KeyPointerPtr keypointerPtr(new KeyPointer(*this, gpuIndex, _channelParam));
        if (keypointerPtr)
        {
            keypointerPtr->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);

            WaitNotify(__FUNCTION__);

            _keypointers.push_back(keypointerPtr);
        }
    }
}

void FaceDetector::StopKeypointers()
{
    for each (KeyPointerPtr keypointerPtr in _keypointers)
    {
        keypointerPtr->ReadyToStop();
        keypointerPtr->Stop();
    }
    _keypointers.clear();
}

void FaceDetector::StartOneAligner(int gpuIndex) throw(BaseException)
{
    for (int idx = 0; idx < _alignParam.threadCount; ++idx)
    {
        AlignerPtr alignerPtr(new Aligner(*this, gpuIndex, _channelParam));
        if (alignerPtr)
        {
            alignerPtr->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);

            WaitNotify(__FUNCTION__);

            _aligners.push_back(alignerPtr);
        }
    }
}

void FaceDetector::StopAligners()
{
    for each (AlignerPtr alignerPtr in _aligners)
    {
        alignerPtr->ReadyToStop();
        alignerPtr->Stop();
    }
    _aligners.clear();
}

void FaceDetector::StartOneAnalyzer(int gpuIndex) throw(BaseException)
{
    if (_analyzeParam.analyze_age_ethnic
        || _analyzeParam.analyze_age_gender
        || _analyzeParam.analyze_glasses
        || _analyzeParam.analyze_mask)
    {
        for (int idx = 0; idx < _analyzeParam.threadCount; ++idx)
        {
            FaceAttrAnalyzerPtr faceAttrAnalyzerPtr(new FaceAttrAnalyzer(*this, gpuIndex, _channelParam));
            if (faceAttrAnalyzerPtr)
            {
                faceAttrAnalyzerPtr->Start(gpuIndex >= 0 ? GpuCtxIndex::Next(gpuIndex) : 0);
                _faceAttrAnalyzerPtrs.push_back(faceAttrAnalyzerPtr);
            }

            WaitNotify(__FUNCTION__);
        }
    }
}

void FaceDetector::StopAnalyzers()
{
    for each (FaceAttrAnalyzerPtr faceAttrAnalyzerPtr in _faceAttrAnalyzerPtrs)
    {
        faceAttrAnalyzerPtr->ReadyToStop();
        faceAttrAnalyzerPtr->Stop();
    }
    _faceAttrAnalyzerPtrs.clear();
}

size_t FaceDetector::PushBuffer(ApiImagePtrBuffer& buffer, size_t& size, int threshold, ApiImagePtrBuffer& addedBuffer, int addedSize)
{
    size += addedSize;
    buffer.splice(buffer.end(), addedBuffer);

    size_t poppedSize = 0;
    if (size > threshold)
    {
        poppedSize = size - threshold;

        ApiImagePtrBuffer::iterator newbegin = buffer.begin();
        std::advance(newbegin, poppedSize);
        buffer.erase(buffer.begin(), newbegin);

        size = threshold;
    }
    return poppedSize;
}

bool FaceDetector::FetchBatch(ApiImagePtrBuffer& buffer, size_t& size, int threshold, ApiImagePtrBatch& apiImageIPtrBatch)
{
    size_t fetchedSize = size >= threshold ? threshold : size;

    ApiImagePtrBuffer::iterator newbegin = buffer.begin();
    std::advance(newbegin, fetchedSize);

    std::copy(buffer.begin(), newbegin, std::back_inserter(apiImageIPtrBatch));
    buffer.erase(buffer.begin(), newbegin);

    size -= fetchedSize;

    return fetchedSize > 0;
}

void FaceDetector::PushOneDetect(ApiImagePtrBuffer& apiImageIPtrBuffer, int size)
{
    if (_detectParam.threadCount > 0)
    {
        AUTOLOCK(_detectBufferLocker);
        size_t poppedSize = PushBuffer(_detectBuffer, _detectImageNumber, _detectParam.bufferSize, apiImageIPtrBuffer, size);
        if (poppedSize > 0)
        {
            LOG(WARNING) << poppedSize << " images in detect on Gpu:" << _detectParam.deviceIndex << " were discarded, because of buffer overflow: " << _detectParam.bufferSize;
        }

        WAKEUP_ALL(_detectBufferCondition);
    }
}

bool FaceDetector::FetchOneDetect(ApiImagePtrBatch& apiImageIPtrBatch)
{
    WAIT_MILLISEC_TILL_COND(_detectBufferCondition, _detectBufferLocker, _detectParam.batchTimeout, [this](){ return _detectImageNumber >= _detectParam.batchSize; });

    if (_detectImageNumber > 0)
    {
        return FetchBatch(_detectBuffer, _detectImageNumber, _detectParam.batchSize, apiImageIPtrBatch);
    }

    return false;
}

void FaceDetector::PushOneTrack(ApiImagePtrBuffer& apiImageIPtrBuffer, int size)
{
    AUTOLOCK(_trackBufferLocker);
    for each (ApiImagePtr apiImagePtr in apiImageIPtrBuffer)
    {
        // full detect and track or track only image
        int detectInterval = apiImagePtr->faceParam.detect_interval;
        if (detectInterval <= 1 || !apiImagePtr->needetect)
        {
            _trackBuffer.push_back(apiImagePtr);
            _trackImageNumber++;
        } 
        else
        {
            bool processed = false;
            for each (ApiImagePtr buff in _trackBuffer)
            {
                // attach face information to the latest one image
                FaceSdkBoxes& sdkBoxes = buff->sdkBoxes;
                if (buff->sourceId == apiImagePtr->sourceId && sdkBoxes.empty() && buff->timestamp < apiImagePtr->timestamp)
                {
                    sdkBoxes.swap(apiImagePtr->sdkBoxes);
                    processed = true;
                    break;
                }
            }

            // if not found any latest image, then add this to buffer
            if (!processed)
            {
                _trackBuffer.push_back(apiImagePtr);
                _trackImageNumber++;
            }
        }
    }

    // discard overflowed images
    size_t poppedSize = 0;
    const int bufferSize = _trackParam.bufferSize;
    if (_trackImageNumber > bufferSize)
    {
        poppedSize = _trackImageNumber - bufferSize;

        ApiImagePtrBuffer::iterator newbegin = _trackBuffer.begin();
        std::advance(newbegin, poppedSize);
        _trackBuffer.erase(_trackBuffer.begin(), newbegin);

        _trackImageNumber = bufferSize;
    }

    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " images in track on Gpu:" << _trackParam.deviceIndex << " were discarded, because of buffer overflow: " << bufferSize;
    }
    WAKEUP_ALL(_trackBufferCondition);
}

bool FaceDetector::FetchOneTrack(ApiImagePtrBatch& apiImageIPtrBatch)
{
    WAIT_MILLISEC_TILL_COND(_trackBufferCondition, _trackBufferLocker, _trackParam.batchTimeout, [this](){ return _trackImageNumber >= _trackParam.batchSize; });

    if (_trackBuffer.size() > 0)
    {
        ApiImagePtrBuffer::iterator it = _trackBuffer.begin();
        int fetchedSize = 0;
        while (it != _trackBuffer.end() && fetchedSize < _trackParam.batchSize)
        {
            ApiImagePtr& alias = *it;
            bool found = false;
            for each (ApiImagePtr apiImagePtr in apiImageIPtrBatch)
            {
                if (apiImagePtr->sourceId == alias->sourceId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                apiImageIPtrBatch.push_back(alias);
                it = _trackBuffer.erase(it);
                _trackImageNumber--;
            }
            else
            {
                ++it;
            }
        }
    }

    return apiImageIPtrBatch.size() > 0;
}

void FaceDetector::PushOneEvaluates(ApiImagePtrBuffer& apiImageIPtrBuffer, int size)
{
    AUTOLOCK(_evaluateBufferLocker);
    size_t poppedSize = PushBuffer(_evaluateBuffer, _evaluateImageNumber, _evaluateParam.bufferSize, apiImageIPtrBuffer, size);
    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " images in badness evaluation on Gpu:" << _evaluateParam.deviceIndex << " were discarded, because of buffer overflow: " << _evaluateParam.bufferSize;
    }
    WAKEUP_ALL(_evaluateBufferCondition);
}

bool FaceDetector::FetchOneEvaluates(ApiImagePtrBatch& apiImageIPtrBatch)
{
    WAIT_MILLISEC_TILL_COND(_evaluateBufferCondition, _evaluateBufferLocker, _evaluateParam.batchTimeout, [this](){ return _evaluateImageNumber >= _evaluateParam.batchSize; });

    if (_evaluateImageNumber > 0)
    {
        return FetchBatch(_evaluateBuffer, _evaluateImageNumber, _evaluateParam.batchSize, apiImageIPtrBatch);
    }

    return false;
}

void FaceDetector::PushOneDetectKeypoints(ApiImagePtrBuffer& apiImageIPtrBuffer, int size)
{
    AUTOLOCK(_keyPointsBufferLocker);
    long long poppedSize = PushBuffer(_keyPointsBuffer, _keypointsImageNumber, _keypointParam.bufferSize, apiImageIPtrBuffer, size);
    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " images in keypoints on Gpu:" << _keypointParam.deviceIndex << " were discarded, because of buffer overflow: " << _keypointParam.bufferSize;
    }
    WAKEUP_ALL(_keyPointsBufferCondition);
}

bool FaceDetector::FetchOneDetectKeypoints(ApiImagePtrBatch& apiImageIPtrBatch)
{
    WAIT_MILLISEC_TILL_COND(_keyPointsBufferCondition, _keyPointsBufferLocker, _keypointParam.batchTimeout, [this](){ return _keypointsImageNumber >= _keypointParam.batchSize; });

    if (_keypointsImageNumber > 0)
    {
        return FetchBatch(_keyPointsBuffer, _keypointsImageNumber, _keypointParam.batchSize, apiImageIPtrBatch);
    }

    return false;
}

void FaceDetector::PushOneAlign(ApiImagePtrBuffer& apiImageIPtrBuffer, int size)
{
    AUTOLOCK(_alignBufferLocker);
    long long poppedSize = PushBuffer(_alignBuffer, _alignImageNumber, _alignParam.bufferSize, apiImageIPtrBuffer, size);
    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " images in align on Gpu:" << _alignParam.deviceIndex << " were discarded, because of buffer overflow: " << _alignParam.bufferSize;
    }

    WAKEUP_ALL(_alignBufferCondition);
}

bool FaceDetector::FetchOneAlign(ApiImagePtrBatch& apiImageIPtrBatch)
{
    WAIT_MILLISEC_TILL_COND(_alignBufferCondition, _alignBufferLocker, _alignParam.batchTimeout, [this](){ return _alignImageNumber >= _alignParam.batchSize; });

    if (_alignImageNumber > 0)
    {
        return FetchBatch(_alignBuffer, _alignImageNumber, _alignParam.batchSize, apiImageIPtrBatch);
    }

    return false;
}

void FaceDetector::PushOneAnalyze(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size)
{
    AUTOLOCK(_faceAttrAnalyzeBufferLocker);
    long long poppedSize = FaceExtractor::PushBuffer(_faceAttrAnalyzeBuffer, _faceNumberToAnalyze, _analyzeParam.bufferSize, analyzeResultPtrBuffer, size);
    if (poppedSize > 0)
    {
        LOG(WARNING) << poppedSize << " faces in analyze on Gpu:" << _analyzeParam.deviceIndex << " were discarded, because of buffer overflow: " << _analyzeParam.bufferSize;
    }

    WAKEUP_ALL(_faceAttrAnalyzeBufferCondition);
}

bool FaceDetector::FetchOneAnalyze(AnalyzeResultPtrBatch& analyzeResultPtrs)
{
    WAIT_MILLISEC_TILL_COND(_faceAttrAnalyzeBufferCondition, _faceAttrAnalyzeBufferLocker, _analyzeParam.batchTimeout, [this](){ return _faceNumberToAnalyze >= _analyzeParam.batchSize; });

    if (_faceNumberToAnalyze > 0)
    {
        return FaceExtractor::FetchBatch(_faceAttrAnalyzeBuffer, _faceNumberToAnalyze, _analyzeParam.batchSize, analyzeResultPtrs);
    }

    return false;
}

void FaceDetector::PushOneExtract(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer, int size)
{
    if (_faceExtractor)
    {
        _faceExtractor->PushOneExtract(analyzeResultPtrBuffer, size);
    }
}

void FaceDetector::PushOneResults(CaptureResults& captureResults)
{
    AUTOLOCK(_outputBufferLocker);
    _outputBuffer.push(captureResults);
    _resultFaceNumber += captureResults.size();

    long long poppedSize = 0;
    while (_resultFaceNumber > _resultParam.bufferSize)
    {
        long long headSize = _outputBuffer.front().size();
        _outputBuffer.pop();
        poppedSize += headSize;
    }

    if (poppedSize > 0)
    {
        _resultFaceNumber -= poppedSize;
        LOG(WARNING) << poppedSize << " faces in detect result buffer were discarded, because of buffer overflow: " << _resultParam.bufferSize;
    }
}

void FaceDetector::PushBests(AnalyzeResultPtrBuffer& analyzeResultPtrBuffer)
{
    if (_faceExtractor)
    {
        AnalyzeResultPtrBuffer toExtractAnalyzePtrBuffer;
        size_t toExtractAnalyzePtrBufferSize = 0;
        CaptureResults captureResults;
        for each (AnalyzeResultPtr analyzeResultPtr in analyzeResultPtrBuffer)
        {
            if (analyzeResultPtr)
            {
                const FaceParam& facePara = analyzeResultPtr->faceParam;
                if (facePara.extract_feature)
                {
                    toExtractAnalyzePtrBuffer.push_back(analyzeResultPtr);
                    toExtractAnalyzePtrBufferSize++;
                } 
                else
                {
                    captureResults.push_back(analyzeResultPtr->captureResultPtr);
                }
            }
        }
        
        if (toExtractAnalyzePtrBufferSize > 0)
        {
            PushOneExtract(toExtractAnalyzePtrBuffer, toExtractAnalyzePtrBufferSize);
        }

        if (captureResults.size() > 0)
        {
            PushOneResults(captureResults);
        }
    } 
    else
    {
        CaptureResults captureResults;
        for each (AnalyzeResultPtr analyzeResultPtr in analyzeResultPtrBuffer)
        {
            if (analyzeResultPtr)
            {
                captureResults.push_back(analyzeResultPtr->captureResultPtr);
            }
        }
        
        if (captureResults.size() > 0)
        {
            PushOneResults(captureResults);
        }
    }
}

void FaceDetector::StartPrepareDetectBuffer()
{
    if (!_preparingDetectBuffer && _error_code == 0)
    {
        _preparingDetectBuffer = true;
        _prepareDetectBuffer = std::thread(&FaceDetector::PrepareDetectBuffer, this);
    }
}

void FaceDetector::PrepareDetectBuffer()
{
    while (_preparingDetectBuffer)
    {
        START_FUNCTION_EVALUATE();
        
        ApiImagePtrBuffer detectBuffer, trackBuffer;
        int detectBufferSize = 0, trackBufferSize = 0;
        START_EVALUATE(FetchDecodedFrame);
        {
            AUTOLOCK(_contextLocker);
            for each(BaseDecoder* baseDecoder in _baseDecoders)
            {
                DecodedFrame decodedFrame;
                if (baseDecoder && baseDecoder->GetFrame(decodedFrame))
                {
                    const FaceParam& faceParam = baseDecoder->GetFaceParam();
                    
                    // convert to API image
                    ApiImagePtr apiImagePtr = FromDecodedFrame(decodedFrame, faceParam);
                    if (apiImagePtr)
                    {
                        //apiImagePtr->Show();

                        if (apiImagePtr->needetect || _trackParam.threadCount <= 0)
                        {
                            detectBuffer.push_back(apiImagePtr);
                            detectBufferSize++;
                        }
                        else
                        {
                            trackBuffer.push_back(apiImagePtr);
                            trackBufferSize++;
                        }
                    }
                }
            }
        }

        // if no frame decoded, then wait for 1ms
        if (trackBufferSize == 0 && detectBufferSize == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        PRINT_COSTS(FetchDecodedFrame);

        if (_preparingDetectBuffer && detectBufferSize > 0)
        {
            PushOneDetect(detectBuffer, detectBufferSize);
        }

        if (_preparingDetectBuffer && trackBufferSize > 0)
        {
            PushOneTrack(trackBuffer, trackBufferSize);
        }

        PRINT_FUNCTION_COSTS();
    }
}

void FaceDetector::StopPrepareDetectBuffer()
{
    if (_preparingDetectBuffer)
    {
        _preparingDetectBuffer = false;
        WAIT_TO_EXIT(_prepareDetectBuffer);
    }
}

ApiImagePtr FaceDetector::FromDecodedFrame(DecodedFrame& decodeFrame, const FaceParam& faceParam)
{
    ApiImagePtr apiImagePtr = nullptr;
    if (!decodeFrame.gpumat.empty())
    {
        apiImagePtr = ApiImagePtr(new ApiGpuMat(faceParam, decodeFrame.sourceId, decodeFrame.id, decodeFrame.gpumat, _detectParam.deviceIndex, decodeFrame.timestamp, decodeFrame.buffered));
        apiImagePtr->position = decodeFrame.position;
        apiImagePtr->needetect = decodeFrame.needDetect;
    }
    else if (!decodeFrame.mat.empty())
    {
        apiImagePtr = ApiImagePtr(new ApiMat(faceParam, decodeFrame.sourceId, decodeFrame.id, decodeFrame.mat, _detectParam.deviceIndex, decodeFrame.timestamp, decodeFrame.buffered));
        apiImagePtr->position = decodeFrame.position;
        apiImagePtr->needetect = decodeFrame.needDetect;
    }
    else if (decodeFrame.imdata && !decodeFrame.imdata->empty())
    {
        apiImagePtr = ApiImagePtr(new ApiRaw(faceParam, decodeFrame.sourceId, decodeFrame.id, decodeFrame.imdata, _detectParam.deviceIndex, decodeFrame.timestamp, decodeFrame.buffered));
        apiImagePtr->position = decodeFrame.position;
        apiImagePtr->needetect = decodeFrame.needDetect;
    }
    return apiImagePtr;
}

void FaceDetector::InitializingCurrentState()
{
    _error_code = 0;
}

void FaceDetector::NotifyMe(int err)
{
    _error_code = err;
    WAKEUP_ONE(_oneWorkerReady);
}

void FaceDetector::WaitNotify(const char* msg) throw(BaseException)
{
    // wait the working thread to wake up to stop wait
    WAIT(_oneWorkerReady, _oneWorkerReadyLocker);

    // if failed, then stop
    if (_error_code)
    {
        throw BaseException(_error_code, std::string(msg) + " failed, error code: " + std::to_string(_error_code));
    }
}



