
#include "StreamDecoder.h"
#include "DecodeManager.h"

#include "CudaOperation.h"

#include "dxva2/ffmpeg_dxva2.h"

static char LAST_DECODE_ERROR[512] = { 0 };

STREAMDECODER_API bool DecodeInit()
{
    static bool INITIALIZED = false;
    if (!INITIALIZED)
    {
        INITIALIZED = true;
        int res = DecodeManager::GetLicenseInformation();
        if (res)
        {
            std::string err = "Get decode license information failed, error code: " + std::to_string(res);
            memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
            strcpy(LAST_DECODE_ERROR, err.c_str());
            INITIALIZED = false;
            return false;
        }

        avcodec_register_all();
        av_register_all();
        avformat_network_init();
        avdevice_register_all();
    }

    CudaOperation::Initialize();
    CallbackPool::InitializeCallbackContext();

    return true;
}

STREAMDECODER_API void DecodeDestroy()
{
    CallbackPool::DestroyCallbackContext();
    CudaOperation::Destroy();
}

STREAMDECODER_API const char* GetLastDecodeError()
{
    return LAST_DECODE_ERROR;
}

STREAMDECODER_API const char* GetDecoderId(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        return baseDecoder->Id().c_str();
    }
    return nullptr;
}

STREAMDECODER_API const int GetDecoderDeviceIndex(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        return baseDecoder->GetDeviceIndex();
    }
    return -1;
}

STREAMDECODER_API bool DecodeFrame(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        return baseDecoder->DecodeFrame();
    }
    return false;
}

STREAMDECODER_API BaseDecoder* OpenRTSP(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, bool async, DecoderStoppedCallback stoppedCb)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeManager::OpenRTSP(url, decoderParam, decodeParam, id, async);
        baseDecoder->SetStoppedCallback(stoppedCb);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_API BaseDecoder* OpenVideo(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, bool async, DecoderStoppedCallback stoppedCb)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeManager::OpenVideo(url, decoderParam, decodeParam, id, async);
        baseDecoder->SetStoppedCallback(stoppedCb);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_API BaseDecoder* OpenUSB(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, DecoderStoppedCallback stoppedCb)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeManager::OpenUSB(url, decoderParam, decodeParam, id);
        baseDecoder->SetStoppedCallback(stoppedCb);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_API BaseDecoder* OpenDirectory(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, DecoderStoppedCallback stoppedCb)
{
    BaseDecoder* baseDecoder = nullptr;
    try
    {
        baseDecoder = DecodeManager::OpenDirectory(url, decoderParam, decodeParam, id);
        baseDecoder->SetStoppedCallback(stoppedCb);
    }
    catch (BaseException& ex)
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        memcpy(LAST_DECODE_ERROR, ex.Description().c_str(), ex.Description().size());
    }
    return baseDecoder;
}

STREAMDECODER_API bool CloseDecoder(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        DecodeManager::CloseDecoder(baseDecoder);
    }
    else
    {
        memset(LAST_DECODE_ERROR, 0, sizeof(LAST_DECODE_ERROR));
        strcpy(LAST_DECODE_ERROR, "BaseDecoder is nullptr");
    }
    return false;
}


