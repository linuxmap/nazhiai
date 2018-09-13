
#include "StreamDecoder.h"
#include "DecodeTask.h"

#include "dxva2/ffmpeg_dxva2.h"

#include <map>
#include <memory>

static char LAST_DECODE_ERROR[512] = { 0 };

STREAMDECODER_C_API bool DecodeInit()
{
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    avdevice_register_all(); 
    return true;
}

STREAMDECODER_C_API void DecodeDestroy()
{
}

STREAMDECODER_C_API const char* GetLastDecodeError(int err)
{
    return LAST_DECODE_ERROR;
}

STREAMDECODER_C_API BaseDecoder* OpenRTSP(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const char* id)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeTask::OpenRTSP(url, decoderParam, decodeParam, id);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_C_API bool CloseRTSP(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        try
        {
            baseDecoder->Destroy();
            return true;
        }
        catch (BaseException& ex)
        {
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
    } 
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "BaseDecoder is nullptr");
    }
    return false;
}

STREAMDECODER_C_API BaseDecoder* OpenVideo(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const char* id)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeTask::OpenVideo(url, decoderParam, decodeParam, id);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_C_API bool CloseVideo(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        try
        {
            baseDecoder->Destroy();
            return true;
        }
        catch (BaseException& ex)
        {
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "BaseDecoder is nullptr");
    }
    return false;
}

STREAMDECODER_C_API BaseDecoder* OpenUSB(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const char* id)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeTask::OpenUSB(url, decoderParam, decodeParam, id);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_C_API bool CloseUSB(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        try
        {
            baseDecoder->Destroy();
            return true;
        }
        catch (BaseException& ex)
        {
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "BaseDecoder is nullptr");
    }
    return false;
}

STREAMDECODER_C_API BaseDecoder* OpenDirectory(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const char* id)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeTask::OpenDirectory(url, decoderParam, decodeParam, id);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_C_API bool CloseDirectory(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        try
        {
            baseDecoder->Destroy();
            return true;
        }
        catch (BaseException& ex)
        {
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
        }
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "BaseDecoder is nullptr");
    }
    return false;
}


