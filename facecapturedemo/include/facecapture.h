/*****************************************************************************
*                                                                            *
*  @file     facecapture.h                                                   *
*  @brief    face capture interface                                          *
*  Details.  declare all usable face capture interfaces                      *
*                                                                            *
*  @author   Chico.Wu                                                        *
*  @email    zhigao.wu@nazhiai.com                                           *
*  @version  1.0.1.0507                                                      *
*  @date     2018/05/07                                                      *
*  @license  GNU General Public License (GPL)                                *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : first usable steady version                              *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version>    | <Author>      | <Description>                 *
*----------------------------------------------------------------------------*
*  2018/05/07 | 1.0.1.0507   | Chico.Wu      |                               *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef _FACECAPTURE_HEADER_H_
#define _FACECAPTURE_HEADER_H_

#ifdef FACECAPUTRE_EXPORTS
#define FACECAPUTRE_C_API extern "C" __declspec(dllexport)
#define FACECAPUTRE_API extern __declspec(dllexport)
#define FACECAPUTRE_CLASS __declspec(dllexport)
#else
#define FACECAPUTRE_C_API extern "C" __declspec(dllimport)
#define FACECAPUTRE_API extern __declspec(dllimport)
#define FACECAPUTRE_CLASS __declspec(dllimport)
#endif

#include "FaceCaptureStruct.h"
#include "FaceDetectStruct.h"
#include "StreamDecodeStruct.h"
#include "SnapStruct.h"

typedef void(*StreamAsyncCallback)(const std::string&, const std::string&);

FACECAPUTRE_C_API const char* GetLastCaptureError();

FACECAPUTRE_C_API bool FaceCaptureInit(const char* path = "properties.json");
FACECAPUTRE_C_API void FaceCaptureUnInit();

FACECAPUTRE_C_API bool OpenSnapCamera(int deviceIndex, int cameraType, const char* ip, unsigned short port, const char* username, const char* password, const FaceParam& faceParam);
FACECAPUTRE_C_API bool CloseSnapCamera(const char* ip, unsigned short port);

FACECAPUTRE_C_API bool OpenImageCamera(int deviceIndex, const char* id, const FaceParam& faceParam);
FACECAPUTRE_C_API bool FeedImageCamera(const ImageParam& imageParam);
FACECAPUTRE_C_API bool CloseImageCamera(const char* id);

FACECAPUTRE_C_API bool OpenStreamsAsync(const std::vector<int>& types, const std::vector<std::string>& urls, const std::vector<DecoderParam>& decoderParams, const std::vector<DecodeParam>& decodeParams, const std::vector<FaceParam>& faceParams, const std::vector<std::string>& ids, StreamAsyncCallback callback = nullptr);
FACECAPUTRE_C_API bool CloseStreamsAsync(const std::vector<std::string>& ids, StreamAsyncCallback callback = nullptr);

FACECAPUTRE_C_API bool OpenRTSP(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id);
FACECAPUTRE_C_API bool OpenVideo(const char* path, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id);
FACECAPUTRE_C_API bool OpenUSB(const char* port, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id);
FACECAPUTRE_C_API bool OpenDirectory(const char* path, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id);

FACECAPUTRE_C_API bool CloseStream(const char* id);

FACECAPUTRE_C_API bool GetFaceCapture(std::vector<CaptureResultPtr>& captureResults);

#endif

