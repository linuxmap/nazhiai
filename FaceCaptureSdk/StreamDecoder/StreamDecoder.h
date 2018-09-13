
#ifndef _STREAMDECODER_HEADER_H
#define _STREAMDECODER_HEADER_H

#ifdef STREAMDECODER_EXPORTS
#define STREAMDECODER_C_API extern "C" __declspec(dllexport)
#define STREAMDECODER_API extern __declspec(dllexport)
#define STREAMDECODER_CLASS __declspec(dllexport)
#else
#define STREAMDECODER_C_API extern "C" __declspec(dllimport)
#define STREAMDECODER_API extern __declspec(dllimport)
#define STREAMDECODER_CLASS __declspec(dllimport)
#endif

#include "StreamDecodeStruct.h"

class DecodeTask;
class BaseDecoder;

STREAMDECODER_C_API bool DecodeInit();
STREAMDECODER_C_API void DecodeDestroy();

STREAMDECODER_C_API const char* GetLastDecodeError(int err);

STREAMDECODER_C_API BaseDecoder* OpenRTSP(const char* url, const DecoderParam&, const DecodeParam& decodeParam, const char* = "");
STREAMDECODER_C_API bool CloseRTSP(BaseDecoder*);

STREAMDECODER_C_API BaseDecoder* OpenVideo(const char* url, const DecoderParam&, const DecodeParam& decodeParam, const char* = "");
STREAMDECODER_C_API bool CloseVideo(BaseDecoder*);

STREAMDECODER_C_API BaseDecoder* OpenUSB(const char* url, const DecoderParam&, const DecodeParam& decodeParam, const char* = "");
STREAMDECODER_C_API bool CloseUSB(BaseDecoder*);

STREAMDECODER_C_API BaseDecoder* OpenDirectory(const char* url, const DecoderParam&, const DecodeParam& decodeParam, const char* = "");
STREAMDECODER_C_API bool CloseDirectory(BaseDecoder*);

#endif
