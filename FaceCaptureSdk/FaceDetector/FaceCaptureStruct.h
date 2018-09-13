/*****************************************************************************
*                                                                            *
*  @file     FaceCaptureStruct.h                                             *
*  @brief    face capture result definition                                  *
*  Details.  define face capture result structures                           *
*                                                                            *
*  @author   Chico.Wu                                                        *
*  @email    zhigao.wu@nazhiai.com                                           *
*  @version  1.0.2.0517                                                      *
*  @date     2018/05/07                                                      *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         :                                                          *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version>    | <Author>      | <Description>                 *
*----------------------------------------------------------------------------*
*  2018/05/07 | 1.0.1.0507   | Chico.Wu      |                               *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef _FACECAPTURE_STRUCT_HEADER_H_
#define _FACECAPTURE_STRUCT_HEADER_H_

#include <vector>
#include <string>
#include <memory>

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

/**
* @brief define supported snap camera types \n
* enumerate all supported snap camera type
*/
enum { CAMERA_HC, CAMERA_NONE };

/**
* @brief define angle type and index in FaceBox.angles \n
* 
*/
enum { ANGLE_PITCH, ANGLE_YAW, ANGLE_ROLL };

/**
* @brief define captured face information \n
* 
*/
struct FaceBox {
    int id = 0;

    int x = 0;
    int y = 0;
    int width  = 0;
    int height = 0;

    float confidence = -1.0f;
    float keypointsConfidence = -1.0f;

    float badness    = -1.0f;
    float clarity    = -1.0f;
    float brightness = -1.0f;
    
    int age       = -1;
    int age_group = -1;
    int gender    = -1;
    int ethnic    = -1;

    int glasses = -1;
    int mask    = -1;

    std::vector<float> keypoints = {};
    std::vector<float> visibles  = {};
    std::vector<float> angles    = {};

    std::vector<char> feature    = {};
};

/**
* @brief define capture result \n
*
*/
struct CaptureResult {

    cv::Mat scence = {}; // scence image
    cv::Mat face   = {}; // face image
    cv::Mat aligned = {}; // aligned image

    std::string sourceId = "";
    unsigned long long frameId = 0;
    
    long long timestamp = 0;
    long long position = 0;

    FaceBox faceBox      = {};
};

typedef std::shared_ptr<CaptureResult> CaptureResultPtr;

#endif

