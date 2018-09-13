

/*****************************************************************************
*                                                                            *
*  @file     FaceDetectCore.h                                                *
*  @brief    face detecting core parameter definition                        *
*  Details.  define all face detecting engine parameter                      *
*                                                                            *
*  @author   Chico.Wu                                                        *
*  @email    zhigao.wu@nazhiai.com                                           *
*  @version  1.0.2.0517                                                      *
*  @date     2018/09/11                                                      *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         :                                                          *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version>    | <Author>      | <Description>                 *
*----------------------------------------------------------------------------*
*  2018/09/11 | 1.0.1.0507   | Chico.Wu      |                               *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef _FACEDETECTCORE_HEADER_H_
#define _FACEDETECTCORE_HEADER_H_

#include <vector>

/**
* @brief define capture model information \n
*
*/
struct ModelParam
{
    std::string path = "model/";
    std::string name = "dual";
};

/**
* @brief define parameter at detect phase \n
*
*/
struct DetectParam
{
    int deviceIndex = 0;
    int threadCount = 1;

    std::string faceModel = "default";

    int   faceThreshold = 8;
    float threshold = 0.90f;
    int   maxShortEdge = 720;

    int  bufferSize = 30;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define parameter at track phase \n
*
*/
struct TrackParam
{
    int deviceIndex = 0;
    int threadCount = 1;

    float threshold = 0.50f;

    int bufferSize = 30;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define parameter at evaluate phase \n
*
*/
struct EvaluateParam
{
    int deviceIndex = 0;
    int threadCount = 1;

    int bufferSize = 30;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define parameter at keypoint phase \n
*
*/
struct KeypointParam
{
    int deviceIndex = 0;
    int threadCount = 1;

    int bufferSize = 30;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define parameter at align phase \n
*
*/
struct AlignParam
{
    int deviceIndex = 0;
    int threadCount = 1;

    int bufferSize = 30;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define face analyze rules \n
*
*/
struct AnalyzeParam
{
    int  deviceIndex = 0;
    int  threadCount = 0;

    bool analyze_clarity = false;
    bool analyze_brightness = false;

    bool analyze_glasses = false;
    bool analyze_mask = false;
    bool analyze_age_gender = false;
    bool analyze_age_ethnic = false;

    int bufferSize = 80;

    int batchTimeout = 500;
    int batchSize = -1;
};

/**
* @brief define face feature extract rules \n
*
*/
struct ExtractParam
{
    int deviceIndex = 0;
    int threadCount = 0;

    int bufferSize = 80;

    int batchTimeout = 40;
    int batchSize = 40;
};

/**
* @brief define parameter of output result \n
*
*/
struct ResultParam
{
    int bufferSize = 100; // out put buffer size
};

#endif

