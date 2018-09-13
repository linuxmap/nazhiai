

#include "../FaceDetector/StreamDecoder.h"
#include "../FaceDetector/FaceDetector.h"

#include "../Common/FPS.h"

#include "opencv2/opencv.hpp"

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

#include <thread>

#include <windows.h>

class ImageMachineTest
{
public:
    ImageMachineTest(FaceDetector* faceDetector)
        : _faceDetector(faceDetector)
    {}

    void Start()
    {
        _test = std::thread(&ImageMachineTest::Test, this);
    }

    void Test()
    {
        unsigned char* buffer = (unsigned char*)malloc(2 << 20);
        size_t size = 0;
        int fsn = 0;
        while (true)
        {
            if (_faceDetector)
            {
                if (fsn > 9)
                {
                    fsn = 0;
                }
                std::string fname = "D:/Workbench/FaceCaptureSdk/x64/Release/data/NF_20000" + std::to_string(fsn++) + ".jpg";

                FILE* f = fopen(fname.c_str(), "rb");
                if (f)
                {
                    fseek(f, 0, SEEK_END);
                    size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    fread(buffer, size, 1, f);

                    ImageParam imageParam;
                    imageParam.frameId = 0;
                    imageParam.sourceId = "image_machine";
                    imageParam.imageType = 0;
                    imageParam.scenceImage.swap(std::vector<unsigned char>{buffer, buffer + size});

                    if (FeedSnapMachine(_faceDetector, imageParam))
                    {
                        printf("FeedSnapMachine success\n");
                    } 
                    else
                    {
                        printf("FeedSnapMachine failed\n");
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        free(buffer);
    }

private:
    std::thread _test;
    FaceDetector* _faceDetector;
};

int main(int argc, char* argv[])
{
    if (!DetectInit())
    {
        printf("%s\n", GetLastDetectError());
        return -1;
    }
    if (!DecodeInit())
    {
        printf("%s\n", GetLastDecodeError());
        return -1;
    }

    int device_index = 0;

    DetectParam detectParam;
    detectParam.gpuIndex = device_index;
    detectParam.reservedDetectBufferSize = 5;
    detectParam.reservedTrackBufferSize = 5;
    detectParam.reservedKeyPointsBufferSize = 2;
    detectParam.reservedAlignBufferSize = 2;
    detectParam.reservedResultBufferSize = 100;

    ExtractParam extractParam;
    extractParam.batchSize = 8;
    extractParam.gpuIndex = device_index;

    AnalyzeParam analyzeParam;

    FaceParam faceParam;

    BaseDecoder* rtsp = nullptr, *usb0 = nullptr, *usb1 = nullptr, *film = nullptr, *folder = nullptr;
    FaceDetector* detector = CreateDetector(detectParam, extractParam, analyzeParam);

    ImageMachineTest imageMachineTest(detector);
    if (detector)
    {
        DecodeParam decodeParam;

        faceParam.extract_interval = 0;
        faceParam.extract_feature = true;
        faceParam.face_image_range_scale = 0.0f;
        faceParam.capture_interval = 3000;
        faceParam.capture_frame_number = 1;
        faceParam.scence_image_height = 0;
        faceParam.scence_image_timeout = 5000;

        DecoderParam decoderParam;
        decoderParam.codec = CODEC_CUVID;
        decoderParam.synchronize = false;
        decoderParam.device_index = device_index;
        decoderParam.buffer_size = 20;
        decoderParam.protocol = PROTOCL_UDP;
        rtsp = OpenRTSP("rtsp://admin:admin123@192.168.2.133:554", decoderParam, decodeParam, "133_0");
        int count = 0;
        while (count-- > 0)
        {
            rtsp = OpenRTSP("rtsp://192.168.2.37:8554/111", decoderParam, decodeParam, "22_0");
        }
        if (rtsp)
        {
            AddSource(detector, rtsp, faceParam);
        }
        else
        {
            printf("%s", GetLastDecodeError());
        }

        /*usb0 = OpenUSB("0", decoderParam, decodeParam, "usb_00");
        if (usb0)
        {
            AddSource(detector, usb0, faceParam);
        }

        usb1 = OpenUSB("1", decoderParam, decodeParam, "usb_01");
        if (usb1)
        {
            AddSource(detector, usb1, faceParam);
        }*/

        decoderParam.codec = CODEC_NONE;
        decodeParam.fps = 25.0f;
        //film = OpenVideo("E:/test_video.mkv", decoderParam, DecodeParam(), "video_01");
        if (film)
        {
            AddSource(detector, film, faceParam);
        }

        /*decodeParam.fps = 25.0f;
        BaseDecoder* folder = OpenDirectory("E:/Downloads/20171231/", decoderParam, decodeParam, "dir_faces");
        if (folder)
        {
            AddSource(detector, folder, faceParam);
        }*/

        //AddSnapMachine(detector, CAMERA_HC, faceParam, "192.168.2.133", 8000, "admin", "admin123");
        faceParam.detect_interval = 1;
        /*if (AddSnapMachine(detector, faceParam, "image_machine"))
        {
            imageMachineTest.Start();
        }*/
    }

    while (true)
    {
        clock_t bg = clock();
        std::vector<std::shared_ptr<CaptureResult>> captureResults;
        if (GetCapture(detector, captureResults))
        {
            for each (std::shared_ptr<CaptureResult> captureResult in captureResults)
            {
                if (!captureResult->jpegFace.empty())
                {
                    std::string fname(captureResult->sourceId + "_face.jpg");
                    FILE* f = fopen(fname.c_str(), "wb");
                    if (f)
                    {
                        fwrite(captureResult->jpegFace.data(), 1, captureResult->jpegFace.size(), f);
                        fflush(f);
                        fclose(f);
                    }
                }
                if (!captureResult->jpegScence.empty())
                {
                    std::string fname(captureResult->sourceId + "_scence.jpg");
                    FILE* f = fopen(fname.c_str(), "wb");
                    if (f)
                    {
                        fwrite(captureResult->jpegScence.data(), 1, captureResult->jpegScence.size(), f);
                        fflush(f);
                        fclose(f);
                    }
                }
            }
            continue;
        }
        Sleep(1);
    }

    CloseRTSP(rtsp);
    CloseUSB(usb0);
    CloseUSB(usb1);
    CloseDirectory(folder);
    CloseVideo(film);

    DestroyDetector(detector);

    DecodeDestroy();
    DetectDestroy();

    return 0;
}


