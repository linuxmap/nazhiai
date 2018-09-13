#include "ImageProcess.h"

namespace
{
    //旋转图像内容不变，尺寸相应变大  
    IplImage* simpleRotate(IplImage* img, double degree){
        double angle = degree  * CV_PI / 180.0; // 弧度    
        double a = sin(angle), b = cos(angle);
        int width = img->width;
        int height = img->height;
        int width_rotate = int(height * fabs(a) + width * fabs(b));
        int height_rotate = int(width * fabs(a) + height * fabs(b));
        //旋转数组map  
        // [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]  
        // [ m3  m4  m5 ] ===>  [ A21  A22   b2 ]  
        static float map[6];
        static CvMat map_matrix = cvMat(2, 3, CV_32F, map);
        // 旋转中心  
        CvPoint2D32f center = cvPoint2D32f(width / 2, height / 2);
        cv2DRotationMatrix(center, degree, 1.0, &map_matrix);
        map[2] += (width_rotate - width) / 2;
        map[5] += (height_rotate - height) / 2;
        IplImage* img_rotate = cvCreateImage(cvSize(width_rotate, height_rotate), 8, 3);
        //对图像做仿射变换  
        //CV_WARP_FILL_OUTLIERS - 填充所有输出图像的象素。  
        //如果部分象素落在输入图像的边界外，那么它们的值设定为 fillval.  
        //CV_WARP_INVERSE_MAP - 指定 map_matrix 是输出图像到输入图像的反变换，  
        cvWarpAffine(img, img_rotate, &map_matrix, CV_INTER_LINEAR | CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
        return img_rotate;
    }

#pragma warning(push)
#pragma warning(disable: 4244)

    void computeTransMat1(
        const cv::Mat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = vDegree  * CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0.0f, 0.0f);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(sinHeight, 0);
        dstTri[1] = cv::Point2f(sinHeight + cosWidth, sinWidth);
        dstTri[2] = cv::Point2f(0, cosHeight);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinHeight + cosWidth);
        vDstSize.height = static_cast<int>(cosHeight + sinWidth);
    }

    void computeTransMat2(
        const cv::Mat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 90.0)* CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(sinWidth + cosHeight, sinHeight);
        dstTri[1] = cv::Point2f(cosHeight, cosWidth + sinHeight);
        dstTri[2] = cv::Point2f(sinWidth, 0);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinWidth + cosHeight);
        vDstSize.height = static_cast<int>(cosWidth + sinHeight);
    }

    void computeTransMat3(
        const cv::Mat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 180.0)* CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(cosWidth, cosHeight + sinWidth);
        dstTri[1] = cv::Point2f(0, cosHeight);
        dstTri[2] = cv::Point2f(sinHeight + cosWidth, sinWidth);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinHeight + cosWidth);
        vDstSize.height = static_cast<int>(cosHeight + sinWidth);
    }

    void computeTransMat4(
        const cv::Mat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 270.0)  * CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(0, cosWidth);
        dstTri[1] = cv::Point2f(sinWidth, 0);
        dstTri[2] = cv::Point2f(cosHeight, cosWidth + sinHeight);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinWidth + cosHeight);
        vDstSize.height = static_cast<int>(cosWidth + sinHeight);
    }

    void computeTransMat1(
        const cv::cuda::GpuMat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = vDegree  * CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0.0f, 0.0f);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(sinHeight, 0);
        dstTri[1] = cv::Point2f(sinHeight + cosWidth, sinWidth);
        dstTri[2] = cv::Point2f(0, cosHeight);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinHeight + cosWidth);
        vDstSize.height = static_cast<int>(cosHeight + sinWidth);
    }

    void computeTransMat2(
        const cv::cuda::GpuMat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 90.0)* CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(sinWidth + cosHeight, sinHeight);
        dstTri[1] = cv::Point2f(cosHeight, cosWidth + sinHeight);
        dstTri[2] = cv::Point2f(sinWidth, 0);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinWidth + cosHeight);
        vDstSize.height = static_cast<int>(cosWidth + sinHeight);
    }

    void computeTransMat3(
        const cv::cuda::GpuMat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 180.0)* CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(cosWidth, cosHeight + sinWidth);
        dstTri[1] = cv::Point2f(0, cosHeight);
        dstTri[2] = cv::Point2f(sinHeight + cosWidth, sinWidth);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinHeight + cosWidth);
        vDstSize.height = static_cast<int>(cosHeight + sinWidth);
    }

    void computeTransMat4(
        const cv::cuda::GpuMat& vSrc,
        double vDegree,
        cv::Mat& voTransMat,
        cv::Size& vDstSize)
    {
        int    widthSrc = vSrc.cols;
        int    heightSrc = vSrc.rows;
        double angle = (vDegree - 270.0)  * CV_PI / 180.0; // 弧度    
        double sinA = sin(angle);
        double cosA = cos(angle);
        double sinHeight = heightSrc * sinA;
        double cosWidth = widthSrc * cosA;
        double sinWidth = widthSrc * sinA;
        double cosHeight = heightSrc * cosA;

        cv::Point2f srcTri[3];
        cv::Point2f dstTri[3];
        srcTri[0] = cv::Point2f(0, 0);
        srcTri[1] = cv::Point2f(vSrc.cols - 1, 0);
        srcTri[2] = cv::Point2f(0, vSrc.rows - 1);
        dstTri[0] = cv::Point2f(0, cosWidth);
        dstTri[1] = cv::Point2f(sinWidth, 0);
        dstTri[2] = cv::Point2f(cosHeight, cosWidth + sinHeight);
        voTransMat = getAffineTransform(srcTri, dstTri);

        vDstSize.width = static_cast<int>(sinWidth + cosHeight);
        vDstSize.height = static_cast<int>(cosWidth + sinHeight);
    }

#pragma warning(pop)

    nzimg::CImageProcess::EImageError checkDegree(double& voDegree)
    {
        if (std::fabs(voDegree) > 360.0) {
            return nzimg::CImageProcess::eDegree;
        }
        if (voDegree < 0) {
            voDegree += 360.0;
        }
        return nzimg::CImageProcess::eOK;
    }
}


using namespace nzimg;


CImageProcess::CImageProcess()
{
}


CImageProcess::~CImageProcess()
{
}

CImageProcess::EImageError CImageProcess::rotateImage(const cv::Mat& vSrc, cv::Mat& voDst, double vDegree)
{
    EImageError err = eOK;
    if (eOK != (err = checkDegree(vDegree))) {
        return err;
    }

    cv::Mat transMat;
    cv::Size dstSize;
    if (vDegree < 90) {
        computeTransMat1(vSrc, vDegree, transMat, dstSize);
    }
    else if (vDegree < 180) {
        computeTransMat2(vSrc, vDegree, transMat, dstSize);
    }
    else if (vDegree < 270) {
        computeTransMat3(vSrc, vDegree, transMat, dstSize);
    }
    else {
        computeTransMat4(vSrc, vDegree, transMat, dstSize);
    }
    cv::warpAffine(vSrc, voDst, transMat, dstSize);
    return err;
}

nzimg::CImageProcess::EImageError nzimg::CImageProcess::rotateImage(const cv::cuda::GpuMat& vSrc, cv::cuda::GpuMat& voDst, double vDegree)
{
    EImageError err = eOK;
    if (eOK != (err = checkDegree(vDegree))) {
        return err;
    }

    cv::Mat transMat;
    cv::Size dstSize;
    if (vDegree < 90) {
        computeTransMat1(vSrc, vDegree, transMat, dstSize);
    }
    else if (vDegree < 180) {
        computeTransMat2(vSrc, vDegree, transMat, dstSize);
    }
    else if (vDegree < 270) {
        computeTransMat3(vSrc, vDegree, transMat, dstSize);
    }
    else {
        computeTransMat4(vSrc, vDegree, transMat, dstSize);
    }
    cv::cuda::warpAffine(vSrc, voDst, transMat, dstSize);
    return err;
}

