// facecapturedemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "facecapture.h"

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

#include "cJSON.h"

#include <thread>
#include <windows.h>

#ifdef _DEBUG
#pragma comment(lib, "opencv_core320d.lib")
#pragma comment(lib, "opencv_highgui320d.lib")
#pragma comment(lib, "opencv_imgproc320d.lib")
#pragma comment(lib, "opencv_imgcodecs320d.lib")
#pragma comment(lib, "opencv_videoio320d.lib")
#else
#pragma comment(lib, "opencv_core320.lib")
#pragma comment(lib, "opencv_imgcodecs320.lib")
#pragma comment(lib, "opencv_imgproc320.lib")
#pragma comment(lib, "opencv_highgui320.lib")
#pragma comment(lib, "opencv_videoio320.lib")
#endif

#pragma comment(lib, "facecapture.lib")

#pragma warning(disable:4244)

class ImageMachineTest
{
public:
    ImageMachineTest()
        : _snapping(false)
    {}

    void Start()
    {
        _snapping = true;
        _snap = std::thread(&ImageMachineTest::Snap, this);
    }

    void Stop()
    {
        if (_snapping)
        {
            _snapping = false;
            if (_snap.joinable())
            {
                _snap.join();
            }
        }
    }

protected:
    void Snap()
    {
        unsigned char* buffer = (unsigned char*)malloc(2 << 20);
        size_t size = 0;

        //FILE* f = fopen("E:/Downloads/imgA.jpg", "rb");
        FILE* f = fopen("E:/media/stars/spider.jpg", "rb");

        ImageParam paramWithoutFaceInfo;
        paramWithoutFaceInfo.frameId = 0;
        paramWithoutFaceInfo.sourceId = "image_camera";
        paramWithoutFaceInfo.imageType = 0;
        if (f)
        {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            fread(buffer, size, 1, f);
            fclose(f);
            paramWithoutFaceInfo.scenceImage.swap(std::vector<unsigned char>{buffer, buffer + size});
        }

        ImageParam paramWithFaceInfo;
        paramWithFaceInfo.frameId = 0;
        paramWithFaceInfo.sourceId = "image_camera";
        paramWithFaceInfo.imageType = 0;

        f = fopen("E:/media/stars/tooopen.jpg", "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            fread(buffer, size, 1, f);
            fclose(f);
            paramWithFaceInfo.scenceImage.swap(std::vector<unsigned char>{buffer, buffer + size});
            FaceInfo info;
            info.id = 99;
            info.confidence = 0.95f;
            info.x = 303;
            info.y = 55;
            info.width = 435;
            info.height = 548;

            paramWithFaceInfo.faceInfos.push_back(info);
        }
        free(buffer);

        bool flag = true;
        while (_snapping)
        {
            if (flag)
            {
                if (FeedImageCamera(paramWithoutFaceInfo))
                {
                    printf("FeedImageCamera success\n");
                }
                else
                {
                    printf("FeedImageCamera failed\n");
                }
            } 
            else
            {
                if (FeedImageCamera(paramWithFaceInfo))
                {
                    printf("FeedImageCamera success\n");
                }
                else
                {
                    printf("FeedImageCamera failed\n");
                }
            }
            flag = !flag;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

private:
    std::thread _snap;
    volatile bool _snapping;
};

typedef void(*CaptureShowCallback)(std::vector<CaptureResultPtr>&);

static std::map<std::string, char> streams_to_show;
void CallbackShow(std::vector<CaptureResultPtr>& captureResults)
{
    for each (CaptureResultPtr captureResult in captureResults)
    {
        if (!captureResult->scence.empty() && streams_to_show.find(captureResult->sourceId) != streams_to_show.end())
        {
            cv::putText(captureResult->scence, std::to_string(captureResult->faceBox.id) + " - " + std::to_string(captureResult->faceBox.confidence) + " - " + std::to_string(captureResult->faceBox.keypointsConfidence),
                cv::Point(captureResult->faceBox.x, captureResult->faceBox.y),
                cv::FONT_HERSHEY_PLAIN, 1.0f, cv::Scalar(255, 0, 0));
            cv::putText(captureResult->scence, std::to_string(captureResult->position),
                cv::Point(captureResult->faceBox.x, captureResult->faceBox.y + 50),
                cv::FONT_HERSHEY_PLAIN, 1.0f, cv::Scalar(255, 0, 0));

            cv::imshow(captureResult->sourceId, captureResult->scence);
        }
    }
    cv::waitKey(1);
}

class CaptureShow
{
public:
    CaptureShow(unsigned long long duration, CaptureShowCallback callback = CallbackShow)
        : _duration(duration), _callback(callback)
    {}

    void Start()
    {
        _show = std::thread(&CaptureShow::Show, this);
    }

    void WaitToComplete()
    {
        if (_show.joinable())
        {
            _show.join();
        }
    }

protected:
    
    unsigned long long now()
    {
        typedef std::chrono::milliseconds time_precision;
        typedef std::chrono::time_point<std::chrono::system_clock, time_precision> time_point;
        time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
    }

    void Show()
    {
        unsigned long long begc = now();
        while (now() < begc + _duration)
        {
            std::vector<CaptureResultPtr> captureResults;
            if (GetFaceCapture(captureResults))
            {
                if (_callback)
                {
                    _callback(captureResults);
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    std::thread _show;
    unsigned long long _duration;
    CaptureShowCallback _callback;
};

void OpenAsyncCallback(const std::string& id, const std::string& result)
{
    printf("stream: %s open %s\n", id.c_str(), result.c_str());
}

void CloseAsyncCallback(const std::string& id, const std::string& result)
{
    printf("stream: %s close %s\n", id.c_str(), result.c_str());
}

void testOpenCloseMultiGpus()
{
	DecoderParam decoderParam;
	decoderParam.buffer_size = 5;
	decoderParam.codec = CODEC_CUVID;
	decoderParam.device_index = 0;
	decoderParam.protocol = PROTOCL_TCP;

	FaceParam faceParamForStream;
	faceParamForStream.choose_best_interval = 1000;

	std::vector<std::string> urls;
	std::vector<std::string> ids;
	std::vector<int> types;
	std::vector<DecoderParam> decoderParams;
	std::vector<DecodeParam> decodeParams;
	std::vector<FaceParam> faceParams;

	int asyncIndex = 1;
	decoderParam.protocol = PROTOCL_UDP; // RTSP stream from VLC trans-coded
	while (asyncIndex <= 5)
	{
		urls.push_back("rtsp://192.168.2.155:8554/images");
		ids.push_back("async_" + std::to_string(asyncIndex));
		types.push_back(NETWORK_STREAM);
		decoderParams.push_back(decoderParam);
		decodeParams.push_back(DecodeParam());
		faceParams.push_back(faceParamForStream);
		++asyncIndex;
	}

	// -- 1, open streams with asynchronous mode
	if (!OpenStreamsAsync(types, urls, decoderParams, decodeParams, faceParams, ids, OpenAsyncCallback))
	{
		printf("first OpenStreamsAsync request failed\n");
	}

    // wait and show result for 2 minutes
    CaptureShow showFirstStreams(2 * 60 * 1000);
    showFirstStreams.Start();
    showFirstStreams.WaitToComplete();

	urls.clear();
	ids.clear();
	types.clear();
	decoderParams.clear();
	decodeParams.clear();
	faceParams.clear();
    decoderParam.device_index = 1;
	while (asyncIndex <= 10)
	{
		urls.push_back("rtsp://192.168.2.155:8554/images");
		ids.push_back("async_" + std::to_string(asyncIndex));
		types.push_back(NETWORK_STREAM);
		decoderParams.push_back(decoderParam);
		decodeParams.push_back(DecodeParam());
		faceParams.push_back(faceParamForStream);
		++asyncIndex;
	}

	// -- 2, open RTSP with asynchronous mode
	if (!OpenStreamsAsync(types, urls, decoderParams, decodeParams, faceParams, ids, OpenAsyncCallback))
	{
		printf("second OpenStreamsAsync request failed\n");
	}

	// wait and show result for 2 minutes
	CaptureShow showAllStreams(2 * 60 * 1000);
	showAllStreams.Start();
	showAllStreams.WaitToComplete();

	// close streams asynchronous
	if (CloseStreamsAsync(std::vector<std::string>{"async_1", "async_6"}, CloseAsyncCallback))
	{
		printf("first CloseStreamsAsync request success\n");
	}
	else
	{
		printf("first CloseStreamsAsync request failed: %s\n", GetLastCaptureError());
	}	

	// wait and show result for 1 minutes
	CaptureShow showStreamsAfterFirstClose(1 * 60 * 1000);
    showStreamsAfterFirstClose.Start();
    showStreamsAfterFirstClose.WaitToComplete();

    // close streams asynchronous
    if (CloseStreamsAsync(std::vector<std::string>{"async_4", "async_10"}, CloseAsyncCallback))
    {
        printf("second CloseStreamsAsync request success\n");
    }
    else
    {
        printf("second CloseStreamsAsync request failed: %s\n", GetLastCaptureError());
    }

    // wait and show result for 1 minutes
    CaptureShow showStreamsAfterSecondClose(1 * 60 * 1000);
    showStreamsAfterSecondClose.Start();
    showStreamsAfterSecondClose.WaitToComplete();
}

void testOpenClose()
{
	DecoderParam decoderParam;
	decoderParam.buffer_size = 5;
	decoderParam.codec = CODEC_CUVID;
	decoderParam.device_index = 0;
	decoderParam.protocol = PROTOCL_TCP;

	FaceParam faceParamForStream;
    faceParamForStream.choose_best_interval = 1000;

	std::vector<std::string> urls;
	std::vector<std::string> ids;
	std::vector<int> types;
	std::vector<DecoderParam> decoderParams;
	std::vector<DecodeParam> decodeParams;
	std::vector<FaceParam> faceParams;

    DecodeParam decodeParam;
    urls.push_back("rtsp://admin:admin123@192.168.2.64/h264");
    ids.push_back("a1_" + std::to_string(64));
    streams_to_show.insert(std::make_pair("a1_" + std::to_string(64), 1));
    types.push_back(NETWORK_STREAM);
    decoderParams.push_back(decoderParam);
    decodeParams.push_back(decodeParam);
    faceParams.push_back(faceParamForStream);

    urls.push_back("rtsp://admin:admin123@192.168.2.68:554");
    ids.push_back("a1_" + std::to_string(68));
    streams_to_show.insert(std::make_pair("a1_" + std::to_string(68), 1));
    types.push_back(NETWORK_STREAM);
    decoderParams.push_back(decoderParam);
    decodeParams.push_back(decodeParam);
    faceParams.push_back(faceParamForStream);

	// -- 1, open streams with asynchronous mode
	if (!OpenStreamsAsync(types, urls, decoderParams, decodeParams, faceParams, ids, OpenAsyncCallback))
	{
		printf("OpenStreamsAsync request failed\n");
	}

	// -- 2, open RTSP with synchronous mode
	if (!OpenRTSP("rtsp://admin:admin123@192.168.2.68:554", decoderParam, DecodeParam(), faceParamForStream, "s1_68"))
	{
		printf("OpenRTSP failed: %s\n", GetLastCaptureError());
	}

    streams_to_show.insert(std::make_pair("s1_68", 1));

	// -- 3, open video with synchronous mode
	decoderParam.codec = CODEC_NONE; // use CPU decode
	decodeParam.fps = 25;
	if (!OpenVideo("E:/media/videos/market720p25.avi", decoderParam, decodeParam, faceParamForStream, "sv_1"))
	{
		printf("OpenVideo failed: %s\n", GetLastCaptureError());
    }
    streams_to_show.insert(std::make_pair("sv_1", 1));

	// -- 4, open directory with synchronous mode
	faceParamForStream.detect_interval = 1;
	for (int dirIdx = 1; dirIdx <= 2; ++dirIdx)
	{
		std::string path = "E:/media/3faces/face" + std::to_string(dirIdx);
		if (!OpenDirectory(path.c_str(), decoderParam, decodeParam, faceParamForStream, (std::string("sd_") + std::to_string(dirIdx)).c_str()))
		{
			printf("OpenDirectory failed: %s\n", GetLastCaptureError());
        }
        streams_to_show.insert(std::make_pair(std::string("sd_") + std::to_string(dirIdx), 1));
	}

	// wait and show result for 5 minutes
	CaptureShow showAllStreams(5 * 60 * 1000);
	showAllStreams.Start();
	showAllStreams.WaitToComplete();

	// close streams asynchronous
	if (CloseStreamsAsync(std::vector<std::string>{"a1_64"}, CloseAsyncCallback))
	{
		printf("CloseStreamsAsync request success\n");
	}
	else
	{
		printf("CloseStreamsAsync request failed: %s\n", GetLastCaptureError());
	}

	// close streams asynchronous
	if (CloseStreamsAsync(std::vector<std::string>{"a1_68"}, CloseAsyncCallback))
	{
		printf("CloseStreamsAsync request success\n");
	}
	else
	{
		printf("CloseStreamsAsync request failed: %s\n", GetLastCaptureError());
	}

	// close stream synchronous
	if (!CloseStream("s1_68"))
	{
		printf("CloseStream(s1_68) failed: %s\n", GetLastCaptureError());
	}

	// close stream synchronous
	if (!CloseStream("sv_1"))
	{
		printf("CloseStream(sync_video_1) failed: %s\n", GetLastCaptureError());
	}

	// close stream synchronous
	if (!CloseStream("sd_1"))
	{
		printf("CloseStream(sd_1) failed: %s\n", GetLastCaptureError());
	}

	// close stream synchronous
	if (!CloseStream("sd_2"))
	{
		printf("CloseStream(sd_2) failed: %s\n", GetLastCaptureError());
	}

	// wait and show result for 1 minutes
	CaptureShow showStreamsAfterClose(1 * 60 * 1000);
	showStreamsAfterClose.Start();
    showStreamsAfterClose.WaitToComplete();

    streams_to_show.clear();
}

void testVideosWithImageCamera()
{
    DecoderParam decoderParam;
    decoderParam.buffer_size = 5;
    decoderParam.codec = CODEC_CUVID;
    decoderParam.device_index = 0;

    DecodeParam decodeParam;
    decodeParam.fps = 25;
    decodeParam.failureThreshold = 1000;

    FaceParam faceParamForSnap;
    faceParamForSnap.choose_best_interval = 1000;
    //faceParamForSnap.detect_interval = 1;

    int sindex = 0;
    while (++sindex <= 3)
    {
        std::string id = "video_" + std::to_string(sindex);
        std::string url = "E:/media/videos/metros/test" + std::to_string(sindex) + ".avi";
        if (!OpenVideo(url.c_str(), decoderParam, decodeParam, faceParamForSnap, id.c_str()))
        {
            printf("OpenVideo failed: %s\n", GetLastCaptureError());
        }
        streams_to_show.insert(std::make_pair(id, 1));
    }

    // test image camera
    ImageMachineTest imageMachineTest;
    faceParamForSnap.detect_interval = 1;
    faceParamForSnap.capture_interval = 0;
    faceParamForSnap.choose_best_interval = 0;
    if (!OpenImageCamera(0, "image_camera", faceParamForSnap))
    {
        printf("OpenImageCamera(0, \"image_camera\") failed: %s\n", GetLastCaptureError());
    }
    else
    {
        imageMachineTest.Start();
        streams_to_show.insert(std::make_pair("image_camera", 1));
    }

    // wait and show result for 10 minutes
    CaptureShow showStreams(10 * 60 * 1000, CallbackShow);
    // wait and show result for 24 hours
    //CaptureShow showStreams(24 * 60 * 60 * 1000, CallbackShow);
    showStreams.Start();
    showStreams.WaitToComplete();

    imageMachineTest.Stop(); 
    
    if (!CloseImageCamera("image_camera"))
    {
        printf("CloseImageCamera(\"image_camera\") failed: %s\n", GetLastCaptureError());
    }
}

void general_test()
{
    cv::Mat src = cv::imread("E:/media/stars/spider.jpg", cv::IMREAD_COLOR);
    cv::Mat dst = src.clone();// cv::imread("E:/media/stars/tooopen.jpg", cv::IMREAD_COLOR);

    dst.setTo(cv::Scalar(0, 0, 0));
    cv::imshow("src", src);

    cv::Mat srcroi = src(cv::Rect(0, 0, 100, 100));
    srcroi.copyTo(dst(cv::Rect(100, 100, srcroi.cols, srcroi.rows)));
    printf("after clo copyTo mat\n");
    cv::imshow("copyTo", dst);
    cv::waitKey(1000 * 60);
}

static FaceParam defaultFaceParam;

void ReadFaceParam(cJSON *capture, FaceParam& faceParam)
{
    cJSON* jitem = cJSON_GetObjectItem(capture, "scence_image_height");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.scence_image_height = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "face_image_range_scale");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.face_image_range_scale = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "min_face_size");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.min_face_size = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "max_face_size");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.max_face_size = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "detect_interval");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.detect_interval = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "extract_feature");
    if (jitem)
    {
        faceParam.extract_feature = jitem->type == cJSON_True;
    }

    jitem = cJSON_GetObjectItem(capture, "analyze_glasses");
    if (jitem)
    {
        faceParam.analyze_glasses = jitem->type == cJSON_True;
    }

    jitem = cJSON_GetObjectItem(capture, "analyze_mask");
    if (jitem)
    {
        faceParam.analyze_mask = jitem->type == cJSON_True;
    }

    jitem = cJSON_GetObjectItem(capture, "analyze_age_gender");
    if (jitem)
    {
        faceParam.analyze_age_gender = jitem->type == cJSON_True;
    }

    jitem = cJSON_GetObjectItem(capture, "analyze_age_ethnic");
    if (jitem)
    {
        faceParam.analyze_age_ethnic = jitem->type == cJSON_True;
    }

    jitem = cJSON_GetObjectItem(capture, "capture_interval");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.capture_interval = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "capture_frame_number");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.capture_frame_number = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "choose_entry_timeout");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.choose_entry_timeout = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "choose_best_interval");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.choose_best_interval = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "analyze_result_timeout");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.analyze_result_timeout = jitem->valueint;
    }

    jitem = cJSON_GetObjectItem(capture, "angle_pitch");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.angle_pitch = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "angle_yaw");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.angle_yaw = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "angle_roll");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.angle_roll = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "keypointsConfidence");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.keypointsConfidence = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "badness");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.badness = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "clarity");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.clarity = jitem->valuedouble;
    }

    jitem = cJSON_GetObjectItem(capture, "brightness");
    if (jitem && jitem->type == cJSON_Number)
    {
        faceParam.brightness = jitem->valuedouble;
    }
}

void usercallback(const char* id)
{
    printf("%s %s exit\n", __FUNCTION__, id);
}

void ReadStreams(cJSON *streams, const char* modName, std::vector<std::string>& ids, std::vector<std::string>& urls, std::vector<DecoderParam>& decoderParams, std::vector<DecodeParam>& decodeParams, std::vector<FaceParam>& faceParams, std::vector<char>& syncs)
{
    if (streams)
    {
        if (streams->type == cJSON_Array)
        {
            int arrSize = cJSON_GetArraySize(streams);
            if (arrSize > 0)
            {
                for (int idx = 0; idx < arrSize; ++idx)
                {
                    cJSON *json = cJSON_GetArrayItem(streams, idx);
                    if (json)
                    {
                        DecoderParam decoderParam;
                        cJSON *jitem = cJSON_GetObjectItem(json, "gpu_index");
                        if (jitem && jitem->type == cJSON_Number)
                        {
                            decoderParam.device_index = jitem->valueint;
                        }
                        else
                        {
                            continue;
                        }

                        jitem = cJSON_GetObjectItem(json, "codec");
                        if (jitem)
                        {
                            if (jitem->type == cJSON_String)
                            {
                                decoderParam.codec = strcmp(jitem->valuestring, "cuvid") == 0 ? CODEC_CUVID : CODEC_NONE;
                            } 
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "synchronize");
                        bool syn = true;
                        if (jitem)
                        {
                            syn = jitem->type == cJSON_True ? true : false;
                        }

                        jitem = cJSON_GetObjectItem(json, "protocol");
                        if (jitem)
                        {
                            if (jitem->type == cJSON_String)
                            {
                                decoderParam.protocol = strcmp(jitem->valuestring, "tcp") == 0 ? PROTOCL_TCP : PROTOCL_UDP;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "buffer_size");
                        if (jitem && jitem->type == cJSON_Number)
                        {
                            decoderParam.buffer_size = jitem->valueint;
                        }
                        else
                        {
                            continue;
                        }

                        DecodeParam decodeParam;
                        decodeParam.ExitCallback = usercallback;
                        jitem = cJSON_GetObjectItem(json, "skip_frame_interval");
                        if (jitem)
                        {
                            if (jitem->type == cJSON_Number)
                            {
                                decodeParam.skip_frame_interval = jitem->valueint;
                            } 
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "fps");
                        if (jitem)
                        {
                            if (jitem->type == cJSON_Number)
                            {
                                decodeParam.fps = jitem->valuedouble;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "rotate_angle");
                        if (jitem && jitem->type == cJSON_Number)
                        {
                            if (jitem->type == cJSON_Number)
                            {
                                decodeParam.rotate_angle = jitem->valuedouble;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "failureThreshold");
                        if (jitem)
                        {
                            if (jitem->type == cJSON_Number)
                            {
                                decodeParam.failureThreshold = jitem->valueint;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        jitem = cJSON_GetObjectItem(json, "restartTimes");
                        if (jitem && jitem->type == cJSON_Number)
                        {
                            if (jitem->type == cJSON_Number)
                            {
                                decodeParam.restartTimes = jitem->valueint;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        FaceParam faceParam = defaultFaceParam;
                        jitem = cJSON_GetObjectItem(json, "capture");
                        if (jitem && jitem->type == cJSON_Object)
                        {
                            ReadFaceParam(jitem, faceParam);
                        }
                        else
                        {
                            continue;
                        }
                        
                        std::string urlid;
                        jitem = cJSON_GetObjectItem(json, "id");
                        if (jitem && jitem->type == cJSON_String && strlen(jitem->valuestring) > 0)
                        {
                            urlid = jitem->valuestring;
                        }
                        else
                        {
                            continue;
                        }

                        std::string url;
                        jitem = cJSON_GetObjectItem(json, "url");
                        if (jitem && jitem->type == cJSON_String && strlen(jitem->valuestring) > 0)
                        {
                            url = jitem->valuestring;
                        }
                        else
                        {
                            continue;
                        }

                        bool toshow = false;
                        jitem = cJSON_GetObjectItem(json, "show");
                        if (jitem && jitem->type == cJSON_True)
                        {
                            toshow = true;
                        }

                        int copies = 1;
                        jitem = cJSON_GetObjectItem(json, "copy");
                        if (jitem && jitem->type == cJSON_Number)
                        {
                            copies = jitem->valueint;
                        }

                        for (int cpy = 1; cpy <= copies; ++cpy)
                        {
                            std::string cpyid = urlid + "_" + std::to_string(cpy);
                            ids.push_back(cpyid);
                            urls.push_back(url);
                            decoderParams.push_back(decoderParam);
                            decodeParams.push_back(decodeParam);
                            faceParams.push_back(faceParam);
                            syncs.push_back(syn ? 1 : 0);

                            if (toshow)
                            {
                                streams_to_show.insert(std::make_pair(cpyid, 's'));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            printf("%s is not defined as array(start with'[' and end with']')\n", modName);
        }
    }
}

void ReadAndRun(const char* path)
{
    bool success = true;
    char* content = NULL;
    FILE* cfg = fopen(path, "r");
    if (cfg)
    {
        fseek(cfg, 0, SEEK_END);
        long size = ftell(cfg);
        fseek(cfg, 0, SEEK_SET);
        content = (char*)malloc(size + 1);
        fread(content, size, 1, cfg);
        content[size] = 0;
        fclose(cfg);
    }
    else
    {
        success = false;
        printf("Open configuration file failed: %s\n", path);
    }

    if (success)
    {
        cJSON *jroot = cJSON_Parse(content);
        if (jroot)
        {
            cJSON *capture = cJSON_GetObjectItem(jroot, "capture");
            if (capture)
            {
                ReadFaceParam(capture, defaultFaceParam);
            }

            cJSON *rtsps = cJSON_GetObjectItem(jroot, "rtsp");
            if (rtsps)
            {
                std::vector<std::string> ids;
                std::vector<std::string> urls;
                std::vector<DecoderParam> decoderParams;
                std::vector<DecodeParam> decodeParams;
                std::vector<FaceParam> faceParams;
                std::vector<char> syncs;

                ReadStreams(rtsps, "rtsp", ids, urls, decoderParams, decodeParams, faceParams, syncs);

                std::vector<int> atypes;
                std::vector<std::string> aids;
                std::vector<std::string> aurls;
                std::vector<DecoderParam> adecoderParams;
                std::vector<DecodeParam> adecodeParams;
                std::vector<FaceParam> afaceParams;
                for (size_t idx = 0; idx < ids.size(); ++idx)
                {
                    if (syncs[idx] == 0)
                    {
                        atypes.push_back(NETWORK_STREAM);
                        aids.push_back(ids[idx]);
                        aurls.push_back(urls[idx]);
                        adecoderParams.push_back(decoderParams[idx]);
                        adecodeParams.push_back(decodeParams[idx]);
                        afaceParams.push_back(faceParams[idx]);
                    }
                }

                OpenStreamsAsync(atypes, aurls, adecoderParams, adecodeParams, afaceParams, aids, OpenAsyncCallback);

                for (size_t sidx = 0; sidx < ids.size(); ++sidx)
                {
                    if (syncs[sidx])
                    {
                        OpenRTSP(urls[sidx].c_str(), decoderParams[sidx], decodeParams[sidx], faceParams[sidx], ids[sidx].c_str());
                    }
                }
            }

            cJSON *videos = cJSON_GetObjectItem(jroot, "video");
            if (videos)
            {
                std::vector<std::string> ids;
                std::vector<std::string> urls;
                std::vector<DecoderParam> decoderParams;
                std::vector<DecodeParam> decodeParams;
                std::vector<FaceParam> faceParams;
                std::vector<char> syncs;

                ReadStreams(videos, "video", ids, urls, decoderParams, decodeParams, faceParams, syncs);

                for (size_t sidx = 0; sidx < ids.size(); ++sidx)
                {
                    if (syncs[sidx])
                    {
                        OpenVideo(urls[sidx].c_str(), decoderParams[sidx], decodeParams[sidx], faceParams[sidx], ids[sidx].c_str());
                    }
                }
            }

            cJSON *usbs = cJSON_GetObjectItem(jroot, "usb");
            if (usbs)
            {
                std::vector<std::string> ids;
                std::vector<std::string> urls;
                std::vector<DecoderParam> decoderParams;
                std::vector<DecodeParam> decodeParams;
                std::vector<FaceParam> faceParams;
                std::vector<char> syncs;

                ReadStreams(usbs, "usb", ids, urls, decoderParams, decodeParams, faceParams, syncs);

                for (size_t sidx = 0; sidx < ids.size(); ++sidx)
                {
                    if (syncs[sidx])
                    {
                        OpenUSB(urls[sidx].c_str(), decoderParams[sidx], decodeParams[sidx], faceParams[sidx], ids[sidx].c_str());
                    }
                }
            }

            cJSON *dirs = cJSON_GetObjectItem(jroot, "directory");
            if (dirs)
            {
                std::vector<std::string> ids;
                std::vector<std::string> urls;
                std::vector<DecoderParam> decoderParams;
                std::vector<DecodeParam> decodeParams;
                std::vector<FaceParam> faceParams;
                std::vector<char> syncs;

                ReadStreams(dirs, "directory", ids, urls, decoderParams, decodeParams, faceParams, syncs);

                for (size_t sidx = 0; sidx < ids.size(); ++sidx)
                {
                    if (syncs[sidx])
                    {
                        OpenDirectory(urls[sidx].c_str(), decoderParams[sidx], decodeParams[sidx], faceParams[sidx], ids[sidx].c_str());
                    }
                }
            }

            cJSON_Delete(jroot);
            free(content);
            content = NULL;

            // wait and show result for 24 hours
            unsigned long long days = 30;
            days = days * 24 * 60 * 60 * 1000;
            CaptureShow showStreams(days, CallbackShow);
            
            // wait and show result for 1 minute
            //CaptureShow showStreams(60 * 1000, CallbackShow);
            
            showStreams.Start();
            showStreams.WaitToComplete();
        }
        else
        {
            printf("parse %s streams information failed: %s\n", path, cJSON_GetErrorPtr());
        }
    }

    if (content)
    {
        free(content);
        content = NULL;
    }
}

void testUnit(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: facecapturedemo.exe configuration-file\n");
        system("pause");
        return;
    }

    if (FaceCaptureInit(argv[1]))
    {
        testOpenClose();

        //testOpenCloseMultiGpus(); // note: meed multiple nVidia Gpu

        testVideosWithImageCamera();

        FaceCaptureUnInit();
    }
    else
    {
        printf("FaceCaptureInit failed: %s\n", GetLastCaptureError());
    }
}

void testStreams(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("usage: facecapturedemo.exe configuration-file streams-file[json]\n");
        system("pause");
        return;
    }

    if (FaceCaptureInit(argv[1]))
    {
        ReadAndRun(argv[2]);

        FaceCaptureUnInit();
    }
    else
    {
        printf("FaceCaptureInit failed: %s\n", GetLastCaptureError());
    }
}

int main(int argc, char** argv)
{
#if 1
    testUnit(argc, argv);
#else
    testStreams(argc, argv);
#endif

    return 0;
}

