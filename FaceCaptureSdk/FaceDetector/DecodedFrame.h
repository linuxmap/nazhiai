
#ifndef _DECODEDFRAME_HEADER_H_
#define _DECODEDFRAME_HEADER_H_

#include <memory>

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

typedef std::shared_ptr<std::vector<char>> rawptr;

struct DecodedFrame
{
    std::string sourceId = "";
    unsigned long long id = 0;

    long long timestamp = 0;
    long long position = 0;

    rawptr imdata = nullptr;
    cv::Mat mat = {};
    cv::cuda::GpuMat gpumat = {};

    bool needDetect = true;
    bool buffered = false;
};

#endif

