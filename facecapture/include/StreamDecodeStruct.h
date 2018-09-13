/*****************************************************************************
*                                                                            *
*  @file     StreamDecoderStruct.h                                           *
*  @brief    stream decoding parameter definition                            *
*  Details.  define all decoding control parameter                           *
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
*  2018/05/07 | 1.0.1.0507   | Chico.Wu      |                               *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef _STREAMDECODE_STRUCT_HEADER_H_
#define _STREAMDECODE_STRUCT_HEADER_H_

/**
* @brief define transfer layer type when open network stream \n
*
*/
enum { PROTOCL_TCP, PROTOCL_UDP };

/**
* @brief define usable stream codec type \n
*
*/
enum { CODEC_CUVID, CODEC_NONE};

/**
* @brief define usable stream type \n
*
*/
enum { NETWORK_STREAM, LOCAL_FILE_STREAM, USB_CAMERA_STREAM, DIRECOTRY_FILE_STREAM };

/**
* @brief define decoder information \n
*
*/
struct DecoderParam
{
    int  device_index = 0;       // define decoding GPU index(0,1,2...)
    int  codec = CODEC_CUVID;    // define GPU decoding code type
    int  protocol = PROTOCL_TCP; // define network stream connection type(TCP or UDP)
    int  width = 0;              // define decoded image width, 0 indicates original width will be kept
    int  height = 0;             // define decoded image height, 0 indicates original height will be kept
    int  buffer_size = 10;       // define buffer size that how many images can be buffered to be decoded
};

/**
* @brief define decode rules \n
*
*/
struct DecodeParam
{
    int   skip_frame_interval = 0;  // define skip frame size, 0 indicates no frame will be skipped
    float fps = 0.0f;               // define the decoding fps, 0 indicates original fps will be used
    float rotate_angle = 0.0f;      // define the angle that the camera deviates from the scene

    int failureThreshold = 5000;    // define the failure threshold how long(millisecond) the failure reaches, then the decoder will restart 
    int restartTimes = -1;          // how many times the decoder will restart

    void (*ExitCallback)(const char*) = nullptr; // call back after decoder exit
};

#endif

