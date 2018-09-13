
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
#include "DecodedFrame.h"

class BaseDecoder;

/**
* @brief define callback after stream stopped \n
*
*/
typedef void(*DecoderStoppedCallback)(const std::string&, bool async);

STREAMDECODER_API bool DecodeInit();
STREAMDECODER_API void DecodeDestroy();

STREAMDECODER_API const char* GetLastDecodeError();

STREAMDECODER_API const char* GetDecoderId(BaseDecoder*);
STREAMDECODER_API const int GetDecoderDeviceIndex(BaseDecoder*);

STREAMDECODER_API bool DecodeFrame(BaseDecoder*);

STREAMDECODER_API BaseDecoder* OpenRTSP(const std::string& url, const DecoderParam&, const DecodeParam& decodeParam, const std::string&, bool async, DecoderStoppedCallback stoppedCb);
STREAMDECODER_API BaseDecoder* OpenVideo(const std::string& url, const DecoderParam&, const DecodeParam& decodeParam, const std::string&, bool async, DecoderStoppedCallback stoppedCb);
STREAMDECODER_API BaseDecoder* OpenUSB(const std::string& url, const DecoderParam&, const DecodeParam& decodeParam, const std::string&, DecoderStoppedCallback stoppedCb);
STREAMDECODER_API BaseDecoder* OpenDirectory(const std::string& url, const DecoderParam&, const DecodeParam& decodeParam, const std::string&, DecoderStoppedCallback stoppedCb);

STREAMDECODER_API bool CloseDecoder(BaseDecoder*);

#endif
