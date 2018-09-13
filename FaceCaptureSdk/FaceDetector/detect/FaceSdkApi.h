#ifndef _FACESDKAPI_HEADER_H_
#define _FACESDKAPI_HEADER_H_

#include <vector>
#include <string>
#include <memory>
#include <queue>

#include "FaceSdk.h"
#include "FaceDetectStruct.h"
#include "FaceCaptureStruct.h"
#include "DecodedFrame.h"

#include "AutoLock.h"

typedef std::vector<FaceSdkBox> FaceSdkBoxes;
typedef std::vector<FaceSdkBoxes> MultiFaceSdkBoxes;

typedef std::vector<FaceBox> FaceBoxes;
typedef std::vector<FaceBoxes> MultiFaceBoxes;

typedef std::vector<float> Badnesses;

typedef std::vector<float> Clarities;
typedef std::vector<Clarities> MultiClarities;

typedef std::vector<char> Features;

typedef std::vector<float> Brightnesses;
typedef std::vector<int> Ages;
typedef std::vector<int> Genders;
typedef std::vector<int> AgeGroups;
typedef std::vector<int> Ethnics;

typedef std::vector<char> Masks;
typedef std::vector<char> Glasses;

typedef std::string SourceId;
typedef std::vector<SourceId> SourceIds;

typedef unsigned long long FrameId;

typedef std::vector<FaceSdkImage*> FaceSdkImages;

typedef std::vector<char> Raw;
typedef cv::Mat Mat;
typedef cv::cuda::GpuMat GpuMat;

typedef std::vector<Raw> Raws;
typedef std::vector<Mat> Mats;
typedef std::vector<GpuMat> GpuMats;

typedef std::vector<int> FaceBoxIds;
typedef std::vector<cv::Rect> FaceRects;

class ApiImage {
public:
    static void ToFaceBox(FaceBox&, FaceSdkBox&);

public:
    SourceId sourceId;
    FrameId imageId;
    long long timestamp;
    long long position;

    FaceSdkImage* sdkImage;
    FaceSdkBoxes sdkBoxes;
    FaceBoxIds faceBoxIds;

    bool needetect;
    bool portrait;
    bool buffered;

    int deviceIndex;

    cv::Mat origin;
    cv::Mat scence;

    const FaceParam& faceParam;

    ApiImage(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, int devIndex, long long generatedAt, bool toBeBuffered);
    virtual ~ApiImage();

    virtual void Save(const std::string& filename) = 0;
    virtual void Show() = 0;

    virtual int ResolutionType() = 0;
    virtual void KeepScence(cv::cuda::Stream& stream) = 0;
    virtual void KeepFace(cv::cuda::Stream& stream, cv::Mat& face, int x, int y, int width, int height) = 0;

    virtual void UpdatePortraitTrackId();

    void ScaleRect(const cv::Rect&, int maxWidth, int maxHeight, cv::Rect&);
    
private:
    ApiImage(const ApiImage&);
    ApiImage& operator=(const ApiImage&);
};
typedef std::shared_ptr<ApiImage> ApiImagePtr;

class ApiRaw : public ApiImage
{
public:
    typedef rawptr Image;

public:
    Image image;

    ApiRaw(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, Image& img, int devIndex, long long generatedAt, bool toBeBuffered);
    ~ApiRaw();

    void Save(const std::string& filename);
    void Show();

    void KeepScence(cv::cuda::Stream& stream);
    void KeepFace(cv::cuda::Stream& stream, cv::Mat& face, int x, int y, int width, int height);
    int ResolutionType();
};
typedef std::shared_ptr<ApiRaw> ApiRawPtr;

class ApiMat : public ApiImage
{
public:
    typedef Mat Image;

public:
    Image image;
    FaceRects faceRects;

    ApiMat(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, Image& img, int devIndex, long long generatedAt, bool toBeBuffered, const cv::Mat& scenceImage = cv::Mat(), const FaceRects& faceRects = {});
    ~ApiMat();

    void Save(const std::string& filename);
    void Show();

    void KeepScence(cv::cuda::Stream& stream);
    void KeepFace(cv::cuda::Stream& stream, cv::Mat& face, int x, int y, int width, int height);
    int ResolutionType();

    void UpdatePortraitTrackId();
};
typedef std::shared_ptr<ApiMat> ApiMatPtr;

class ApiGpuMat : public ApiImage
{
public:
    typedef GpuMat Image;

public:
    Image image;

    ApiGpuMat(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, GpuMat& img, int devIndex, long long generatedAt, bool toBeBuffered);
    ~ApiGpuMat();

    void Save(const std::string& filename);
    void Show();

    void KeepScence(cv::cuda::Stream& stream);
    void KeepFace(cv::cuda::Stream& stream, cv::Mat& face, int x, int y, int width, int height);
    int ResolutionType();
};
typedef std::shared_ptr<ApiGpuMat> ApiGpuMatPtr;

class AnalyzeResult
{
public:
    AnalyzeResult(int devIndex, CaptureResultPtr& capture, FaceSdkImage* original, const FaceParam& faceParamRef)
        : gpuIndex(devIndex), captureResultPtr(capture), alignedMat(), alignedImage(nullptr), faceParam(faceParamRef)
    {
        if (original)
        {
            GetImageByMat(original, alignedMat);
            alignedMat = alignedMat.clone();
        }

        if (captureResultPtr && !alignedMat.empty())
        {
            captureResultPtr->aligned = alignedMat;
        }
    }

    bool CreateSdkImage(int devIndex)
    {
        bool result = true;
        if (alignedImage && devIndex != gpuIndex)
        {
            DestroyImage(alignedImage);
            alignedImage = nullptr;
        }

        if (alignedImage == nullptr && CreateImageByMat(alignedImage, alignedMat) != FaceSdkOk)
        {
            alignedImage = nullptr;
            result = false;
        }
        return result;
    }

    void DestroySdkImage()
    {
        if (alignedImage)
        {
            DestroyImage(alignedImage);
            alignedImage = nullptr;
        }
    }

    ~AnalyzeResult()
    {
        if (alignedImage)
        {
            DestroyImage(alignedImage);
            alignedImage = nullptr;
        }
    }

public:
    int gpuIndex;
    CaptureResultPtr captureResultPtr;
    cv::Mat alignedMat;
    FaceSdkImage* alignedImage;
    const FaceParam& faceParam;

private:
    AnalyzeResult(const AnalyzeResult&);
    AnalyzeResult& operator=(const AnalyzeResult&);
};

typedef std::shared_ptr<AnalyzeResult> AnalyzeResultPtr;
typedef std::list<AnalyzeResultPtr> AnalyzeResultPtrBuffer;
typedef std::vector<AnalyzeResultPtr> AnalyzeResultPtrBatch;

typedef std::vector<CaptureResultPtr> CaptureResults;
typedef std::queue<CaptureResults> CaptureResultsQueue;

#endif

