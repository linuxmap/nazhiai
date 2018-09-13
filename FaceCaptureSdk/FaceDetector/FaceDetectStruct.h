/*****************************************************************************
*                                                                            *
*  @file     FaceDetectorStruct.h                                            *
*  @brief    face detecting parameter definition                             *
*  Details.  define all face detecting control parameter                     *
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

#ifndef _FACEDETECTOR_STRUCT_HEADER_H_
#define _FACEDETECTOR_STRUCT_HEADER_H_

/**
* @brief define capture rules \n
*
*/
struct FaceParam
{
    int scence_image_height = 0; // 0: keep original resolution, 720: 1280x720, 1080: 1920x1080, other value: will not output scence image

    float face_image_range_scale = 1.0f; // 1.0 means same as the face box size; smaller than 1.0 means smaller than face box else larger than face box; smaller or euqal 0.0 means no face image output

    int min_face_size   = 50;   // minimum face size (pixel)
    int max_face_size   = 1000; // maximum face size (pixel)

    int detect_interval = 5;    // frame 

    int capture_interval     = 0;   // capture interval for each face (ms)
    int capture_frame_number = 3;   // capture frame number in capture interval

    int choose_best_interval = 0;   // choose interval for best face (ms)
    int choose_entry_timeout = 0;   // the duration how long to find the best face after it appear (ms)

    int analyze_result_timeout = 5000; // time that face attribute kept in memory (ms)

    float angle_pitch = 45.0f;    // face pitch angle : smaller than 0.0 indicates ignore this filter, larger than 0.0 indicates angle value between[-angle_pitch, angle_pitch] is valid
    float angle_yaw   = 45.0f;    // face yaw angle : same as angle_pitch
    float angle_roll  = 45.0f;    // face roll angle : same as angle_pitch

    float keypointsConfidence = 0.80f; // face box confidence (0.0 means all detected face)

    bool analyze_glasses = false;
    bool analyze_mask = false;
    bool analyze_age_gender = false;
    bool analyze_age_ethnic = false;

    float badness    = 0.002f;          // face image badness score (negative means all detected face, 0.0001f is recommended)
    float clarity    = -1.0f;          // face clarity (negative means all detected face, 0.10f is recommended)
    float brightness = -1.0f;          // face brightness (negative means all detected face, 0.20f is recommended)

    bool extract_feature = true;
};

#endif

