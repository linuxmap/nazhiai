#pragma once

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include <opencv2\opencv.hpp>
#pragma warning(default:4819)
#pragma warning(default:4996)

#ifdef _DEBUG

#pragma comment(lib, "opencv_cudawarping320d.lib")

#else

#pragma comment(lib, "opencv_cudawarping320.lib")

#endif

namespace nzimg
{
    class CImageProcess
    {
    public:
        enum EImageError
        {
            eOK,
            eSrcImg = 501,
            eDegree = 1001
        };

        CImageProcess();
        ~CImageProcess();

        /**
         * @brief  rotateImage  ��תͼƬ��
         *
         * @param vSrc          ��ʼͼƬ��
         * @param voDst         ��ת���ͼƬ��
         * @param vDegree       �Ƕ��������[-360, 360]������ᱨeDegree��
         *
         * @return   
         */
        static EImageError rotateImage(const cv::Mat& vSrc, cv::Mat& voDst, double vDegree);
        static EImageError rotateImage(const cv::cuda::GpuMat& vSrc, cv::cuda::GpuMat& voDst, double vDegree);
    };

}

