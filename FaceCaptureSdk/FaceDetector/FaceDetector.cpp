
#include "FaceDetector.h"
#include "FaceDetectorImpl.h"

#include "XMatPool.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#ifdef NDEBUG
#pragma comment(lib, "glog.lib")
#else
#pragma comment(lib, "glogd.lib")
#endif

static char LAST_DECODE_ERROR[512] = { 0 };

static bool INITIALIZED = false;

static void FailureWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

FACEDETECTOR_API bool DetectInit()
{
    static bool license_initialized = false;
    if (!license_initialized)
    {
        std::vector<unsigned char> prodInfo{ 0x01, 0x01 };
        FaceSdkResult res = _GetReservedInfo(prodInfo);
        if (FaceSdkOk != res)
        {
            std::string err = "Product license initialize failed, error code: " + std::to_string(res);
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            strcpy(LAST_DECODE_ERROR, err.c_str());
            return false;
        }
        else
        {
            std::vector<unsigned char> reservedInfo{ 0x00, 0x02 };
            res = _GetReservedInfo(reservedInfo);
            if (FaceSdkOk != res)
            {
                std::string err = "Detect initialize failed, error code: " + std::to_string(res);
                memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
                strcpy(LAST_DECODE_ERROR, err.c_str());
                return false;
            }
            else
            {
                license_initialized = true;
            }
        }
    }
    if (!INITIALIZED)
    {
        INITIALIZED = true;

        google::InitGoogleLogging("FaceDetector");
        google::SetStderrLogging(google::GLOG_FATAL);
        FLAGS_log_dir = "logs";
        FLAGS_colorlogtostderr = true;  // Set log color  
        FLAGS_logbufsecs = 0;  // Set log output speed(s)  
        FLAGS_max_log_size = 128;  // Set max log file size  
        FLAGS_stop_logging_if_full_disk = true;  // If disk is full  
        google::InstallFailureWriter(&FailureWriter);
        google::InstallFailureSignalHandler();

        return true;
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "Detect can not be initialized more than once");
        return false;
    }
}

FACEDETECTOR_API void DetectDestroy()
{
    if (INITIALIZED)
    {
        XMatPool<cv::Mat>::Clear();
        XMatPool<cv::cuda::GpuMat>::Clear();

        google::ShutdownGoogleLogging();

        INITIALIZED = false;
    }
}

FACEDETECTOR_API const char* GetLastDetectError()
{
    return LAST_DECODE_ERROR;
}

FACEDETECTOR_API FaceDetector* CreateDetector(const ModelParam& modelParam, const DetectParam& detectParam, const TrackParam& trackParam,
    const EvaluateParam& evaluateParam, const KeypointParam& keypointParam, const AlignParam& alignParam,
    const AnalyzeParam& analyzerParam, const ResultParam& resultParam, FaceExtractor* faceExtractor)
{
    FaceDetector* faceDetector = new FaceDetector(modelParam, detectParam, trackParam, evaluateParam, keypointParam, alignParam, analyzerParam, resultParam, faceExtractor);
    if (faceDetector)
    {
        try
        {
            faceDetector->Start();
        }
        catch (BaseException& ex)
        {
            delete faceDetector;
            faceDetector = nullptr;

            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
        return faceDetector;
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "allocate memory for FaceDetector failed");
    }
    return nullptr;
}

FACEDETECTOR_API void DestroyDetector(FaceDetector* faceDetector)
{
    if (faceDetector)
    {
        faceDetector->Stop();
        delete faceDetector;
    }
}

FACEDETECTOR_API FaceExtractor* CreateExtractor(const ModelParam& modelParam, const ExtractParam& extractParam, const ResultParam& resultParam)
{
    FaceExtractor* faceExtractor = new FaceExtractor(modelParam, extractParam, resultParam);
    if (faceExtractor)
    {
        try
        {
            faceExtractor->Start();
        }
        catch (BaseException& ex)
        {
            delete faceExtractor;
            faceExtractor = nullptr;

            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
        return faceExtractor;
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "allocate memory for FaceExtractor failed");
    }
    return nullptr;
}

FACEDETECTOR_API void DestroyExtractor(FaceExtractor* faceExtractor)
{
    if (faceExtractor)
    {
        faceExtractor->Stop();
        delete faceExtractor;
    }
}

FACEDETECTOR_API void AddSource(FaceDetector* faceDetector, BaseDecoder* baseDecoder, const FaceParam& faceParam)
{
    if (faceDetector && baseDecoder)
    {
        faceDetector->AddDecoder(baseDecoder, faceParam);
    }
}

FACEDETECTOR_API void DelSource(FaceDetector* faceDetector, BaseDecoder* baseDecoder)
{
    if (faceDetector && baseDecoder)
    {
        faceDetector->DeleteDecoder(baseDecoder);
    }
}

FACEDETECTOR_API bool GetCapture(FaceDetector* faceDetector, std::vector<std::shared_ptr<CaptureResult>>& captureResults)
{
    if (faceDetector)
    {
        return faceDetector->GetCapture(captureResults);
    }
    return false;
}

FACEDETECTOR_API bool GetCapture(FaceExtractor* faceExtractor, std::vector<std::shared_ptr<CaptureResult>>& captureResults)
{
    if (faceExtractor)
    {
        return faceExtractor->GetCapture(captureResults);
    }
    return false;
}

