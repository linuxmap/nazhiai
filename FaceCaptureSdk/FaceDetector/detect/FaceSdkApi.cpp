
#include "FaceSdkApi.h"
#include "TimeStamp.h"

#include "XMatPool.h"
#include "Performance.h"

#include "jpeg_codec_util.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#ifdef _DEBUG
#pragma comment(lib, "opencv_core320d.lib")
#pragma comment(lib, "opencv_imgcodecs320d.lib")
#pragma comment(lib, "opencv_imgproc320d.lib")
#pragma comment(lib, "opencv_highgui320d.lib")
#pragma comment(lib, "opencv_videoio320d.lib")
#else
#pragma comment(lib, "opencv_core320.lib")
#pragma comment(lib, "opencv_imgcodecs320.lib")
#pragma comment(lib, "opencv_imgproc320.lib")
#pragma comment(lib, "opencv_highgui320.lib")
#pragma comment(lib, "opencv_videoio320.lib")
#endif

#pragma comment(lib, "FaceSdk.lib")

ApiImage::ApiImage(const FaceParam& faceParamRef, const SourceId& sourceIdRef, FrameId frameId, int devIndex, long long generatedAt, bool toBeBuffered)
    : sourceId(sourceIdRef), imageId(frameId), timestamp(generatedAt), position(0)
    , sdkImage(nullptr), sdkBoxes(), faceBoxIds()
    , needetect(false), portrait(false), buffered(toBeBuffered)
    , deviceIndex(devIndex)
    , origin(), scence()
    , faceParam(faceParamRef)
{}

ApiImage::~ApiImage()
{
    if (sdkImage)
    {
        DestroyImage(sdkImage);
        sdkImage = nullptr;
    }
}

void ApiImage::ScaleRect(const cv::Rect& src, int maxWidth, int maxHeight, cv::Rect& dst)
{
    int centerX = src.x + (src.width >> 1);
    int centerY = src.y + (src.height >> 1);
    dst.width = (int)(src.width*faceParam.face_image_range_scale);
    dst.height = (int)(src.height*faceParam.face_image_range_scale);
    dst.x = centerX - (dst.width >> 1);
    if (dst.x < 0)
    {
        dst.x = 0;
    }
    if (dst.width + dst.x > maxWidth)
    {
        dst.width = maxWidth - dst.x;
    }
    dst.width &= 0xFFFE;

    dst.y = centerY - (dst.height >> 1);
    if (dst.y < 0)
    {
        dst.y = 0;
    }
    if (dst.height + dst.y > maxHeight)
    {
        dst.height = maxHeight - dst.y;
    }
    dst.height &= 0xFFFE;
}

void ApiImage::UpdatePortraitTrackId()
{
    for (size_t idx = 0; idx < sdkBoxes.size() && idx < faceBoxIds.size(); ++idx)
    {
        sdkBoxes[idx].number = faceBoxIds[idx];
    }
}

void ApiImage::ToFaceBox(FaceBox& faceBox, FaceSdkBox& sdkBox)
{
    faceBox.id = sdkBox.number;
    faceBox.x = sdkBox.x;
    faceBox.y = sdkBox.y;
    faceBox.width = sdkBox.width;
    faceBox.height = sdkBox.height;
    faceBox.confidence = sdkBox.confidence;
    faceBox.keypointsConfidence = sdkBox.keypointsConfidence;
    faceBox.badness = sdkBox.badness;
    
    faceBox.visibles.swap(sdkBox.visibles);
    faceBox.angles.swap(sdkBox.angles);
    faceBox.keypoints.swap(sdkBox.keypoints);
}

ApiRaw::ApiRaw(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, Image& img, int devIndex, long long generatedAt, bool toBeBuffered)
    : ApiImage(faceParamRef, sourceId, frameId, devIndex, generatedAt, toBeBuffered), image(img)
{
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, timestamp);

    START_FUNCTION_EVALUATE();
    if (!img || img->empty() || FaceSdkOk != CreateImage(sdkImage, *img))
    {
        sdkImage = nullptr;
    }
    else
    {
        // release memory 
        img.reset();

        // get cv::Mat
        GetImageByMat(sdkImage, origin);
    }
    PRINT_FUNCTION_COSTS();
}

ApiRaw::~ApiRaw()
{
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, TimeStamp<MILLISECONDS>::Now());
}

void ApiRaw::Save(const std::string& filename)
{

}

void ApiRaw::Show()
{
    cv::imshow(sourceId, scence);
    cv::waitKey(1);
}

void ApiRaw::KeepScence(cv::cuda::Stream&)
{
    if (scence.empty())
    {
        int width = 0, height = faceParam.scence_image_height;
        switch (faceParam.scence_image_height)
        {
        case 0:
            width = origin.cols;
            height = origin.rows;
            break;
        case 720:
            width = 1280;
            break;
        case 1080:
            width = 1920;
            break;
        default:
            break;
        }
        if (height > 0 && width > 0 && !origin.empty())
        {
            if ((origin.rows != height || origin.cols != width))
            {
                cv::resize(origin, scence, cv::Size(width, height));
            }
            else
            {
                scence = origin;
            }
        }
    }
}

void ApiRaw::KeepFace(cv::cuda::Stream&, cv::Mat& face, int x, int y, int width, int height)
{
    if (faceParam.face_image_range_scale > 0.0001f && !origin.empty())
    {
        cv::Rect faceRect;
        ScaleRect(cv::Rect(x, y, width, height), scence.cols, scence.rows, faceRect);
        if (faceRect.width > 0 && faceRect.height > 0)
        {
            face = origin(faceRect);
        }
    }
}

int ApiRaw::ResolutionType()
{
    return (origin.cols << 14) + origin.rows;
}

ApiMat::ApiMat(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, Image& img, int devIndex, long long generatedAt, bool toBeBuffered, const cv::Mat& scenceImage/* = cv::Mat()*/, const FaceRects& faceRectsP/* = {}*/)
    : ApiImage(faceParamRef, sourceId, frameId, devIndex, generatedAt, toBeBuffered), image(img), faceRects(faceRectsP)
{
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, timestamp);
    if (img.empty() || FaceSdkOk != CreateImageByMat(sdkImage, img))
    {
        sdkImage = nullptr;
    }
    
    if (scenceImage.empty())
    {
        origin = img;
    } 
    else
    {
        origin = scenceImage;
    }
}

ApiMat::~ApiMat()
{
    if (buffered)
    {
        XMatPool<cv::Mat>::Free(origin);
    }
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, TimeStamp<MILLISECONDS>::Now());
}

void ApiMat::Save(const std::string& filename)
{
    cv::imwrite(filename, image);
}

void ApiMat::Show()
{
    for each (FaceSdkBox face in sdkBoxes)
    {
        cv::rectangle(origin, cv::Rect(face.x, face.y, face.width, face.height), cv::Scalar(0, 255, 0));
    }
    cv::imshow(sourceId, origin);
    cv::waitKey(1);
}

void ApiMat::KeepScence(cv::cuda::Stream&)
{
    if (scence.empty())
    {
        int width = 0, height = faceParam.scence_image_height;
        switch (faceParam.scence_image_height)
        {
        case 0:
            width = origin.cols;
            height = origin.rows;
            break;
        case 720:
            width = 1280;
            break;
        case 1080:
            width = 1920;
            break;
        default:
            break;
        }
        if (height > 0 && width > 0 && !origin.empty())
        {
            if (origin.rows != height || origin.cols != width)
            {
                cv::resize(origin, scence, cv::Size(width, height));
            }
            else
            {
                scence = origin;
            }
        }
    }
}

void ApiMat::KeepFace(cv::cuda::Stream&, cv::Mat& face, int x, int y, int width, int height)
{
    if (faceParam.face_image_range_scale > 0.0001f && !image.empty())
    {
        if (faceRects.size() > 0)
        {
            for each (cv::Rect rect in faceRects)
            {
                if (x >= rect.x && y >= rect.y && width <= rect.width && height <= rect.height)
                {
                    face = image(rect);
                    break;
                }
            }

            if (face.empty())
            {
                cv::Rect faceRect;
                ScaleRect(cv::Rect(x, y, width, height), origin.cols, origin.rows, faceRect);
                if (faceRect.width > 0 && faceRect.height > 0)
                {
                    face = origin(faceRect);
                }
            }
        } 
        else
        {
            if (portrait)
            {
                face = image;
            } 
            else
            {
                cv::Rect faceRect;
                ScaleRect(cv::Rect(x, y, width, height), origin.cols, origin.rows, faceRect);
                if (faceRect.width > 0 && faceRect.height > 0)
                {
                    face = origin(faceRect);
                }
            }
        }
    }
}

int ApiMat::ResolutionType()
{
    return (image.cols << 14) + image.rows;
}

void ApiMat::UpdatePortraitTrackId()
{
    if (portrait)
    {
        if (faceRects.size() == 0)
        {
            ApiImage::UpdatePortraitTrackId();
        }
        else
        {
            for (size_t idx = 0; idx < sdkBoxes.size() && idx < faceBoxIds.size(); ++idx)
            {
                FaceSdkBox& faceBox = sdkBoxes[idx];
                cv::Rect& faceRect = faceRects[idx];
                if (faceBox.x >= faceRect.x && faceBox.y >= faceRect.y && faceBox.width <= faceRect.width && faceBox.height <= faceRect.height)
                {
                    faceBox.number = faceBoxIds[idx];
                    break;
                }
            }
        }
    }
}

ApiGpuMat::ApiGpuMat(const FaceParam& faceParamRef, const SourceId& sourceId, FrameId frameId, Image& img, int devIndex, long long generatedAt, bool toBeBuffered)
    : ApiImage(faceParamRef, sourceId, frameId, devIndex, generatedAt, toBeBuffered), image(img)
{
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, timestamp);
    if (img.empty() || FaceSdkOk != CreateImageByGpuMat(sdkImage, img))
    {
        sdkImage = nullptr;
    }
}

ApiGpuMat::~ApiGpuMat()
{
    if (buffered)
    {
        XMatPool<cv::cuda::GpuMat>::Free(image, deviceIndex);
    }
    TRACK("%s image(%s:%d) at: %lld\n", __FUNCTION__, sourceId, imageId, TimeStamp<MILLISECONDS>::Now());
}

void ApiGpuMat::Save(const std::string& filename)
{
    cv::Mat mat(image);
    cv::imwrite(filename, mat);
}

void ApiGpuMat::Show()
{
    cv::Mat mat(image);
    for each (FaceSdkBox face in sdkBoxes)
    {
        cv::rectangle(mat, cv::Rect(face.x, face.y, face.width, face.height), cv::Scalar(0, 255, 0));
    }

    cv::putText(mat, std::to_string(*image.refcount), cv::Point(image.cols >> 1, 100), cv::FONT_HERSHEY_PLAIN, 2.0f, cv::Scalar(0, 255, 0));

    cv::imshow(sourceId, mat);
    cv::waitKey(1);
}

void ApiGpuMat::KeepScence(cv::cuda::Stream& stream)
{
    if (origin.empty())
    {
        START_EVALUATE(GpuMatAsyncDownload);
        image.download(origin, stream);
        stream.waitForCompletion();
        PRINT_COSTS(GpuMatAsyncDownload);
    }

    if (scence.empty())
    {
        int width = 0, height = faceParam.scence_image_height;
        switch (faceParam.scence_image_height)
        {
        case 0:
            width = origin.cols;
            height = origin.rows;
            break;
        case 720:
            width = 1280;
            break;
        case 1080:
            width = 1920;
            break;
        default:
            break;
        }
        if (height > 0 && width > 0 && !origin.empty())
        {
            if (origin.rows != height || origin.cols != width)
            {
                cv::resize(origin, scence, cv::Size(width, height));
            }
            else
            {
                scence = origin;
            }
        }
    }
}

void ApiGpuMat::KeepFace(cv::cuda::Stream& stream, cv::Mat& face, int x, int y, int width, int height)
{
    if (faceParam.face_image_range_scale > 0.0001f && !origin.empty())
    {
        cv::Rect faceRect;
        ScaleRect(cv::Rect(x, y, width, height), origin.cols, origin.rows, faceRect);
        if (faceRect.width > 0 && faceRect.height > 0)
        {
            face = origin(faceRect);
        }
    }
}

int ApiGpuMat::ResolutionType()
{
    return (image.cols << 14) + image.rows;
}

