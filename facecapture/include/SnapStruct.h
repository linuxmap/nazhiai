
/*****************************************************************************
*                                                                            *
*  @file     SnapStruct.h                                                    *
*  @brief    face snap parameter definition                                  *
*  Details.  define all face snap parameters                                 *
*                                                                            *
*  @author   Chico.Wu                                                        *
*  @email    zhigao.wu@nazhiai.com                                           *
*  @version  1.0.2.0517                                                      *
*  @date     2018/05/17                                                      *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         :                                                          *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version>    | <Author>      | <Description>                 *
*----------------------------------------------------------------------------*
*  2018/05/17 | 1.0.1.0507   | Chico.Wu      |                               *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef _SNAPSTRUCT_HEADER_H_
#define _SNAPSTRUCT_HEADER_H_

#include <vector>
#include <string>

/**
* @brief define face information from snap camera \n
*
*/
struct FaceInfo
{
    int id = 0;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    float confidence = 0.0f;

    std::vector<unsigned char> faceImage = {}; // face image
};

/**
* @brief define image information from snap camera \n
*
*/
struct ImageParam
{
    char imageType = 0;                          // 0: jpeg
    std::vector<unsigned char> scenceImage = {}; // scence image

    std::string sourceId = "";               // snap camera identity
    unsigned int frameId = 0;                // image id in the snap stream
    long long timestamp  = 0;                // timestamp which indicates when the image was snapped

    std::vector<FaceInfo> faceInfos = {};    // face information in the scence image
};

#endif


