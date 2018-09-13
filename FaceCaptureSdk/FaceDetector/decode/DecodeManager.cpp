#include "DecodeManager.h"

#include "DirectoryDecoder.h"
#include "OpencvDecoder.h"
#include "Dxva2Decoder.h"
#include "GpuDecoder.h"
#include "AsyncDecoder.h"

#include "FaceSdk.h"

std::atomic_int DecodeManager::_license_number;

int DecodeManager::GetLicenseInformation()
{
    std::vector<unsigned char> prodInfo{ 0x01, 0x01 };
    FaceSdkResult res = _GetReservedInfo(prodInfo);
    std::vector<unsigned char> codecInfo{ 0x01, 0x01 };
    if (FaceSdkOk == res && FaceSdkOk == _get_codec_info(codecInfo))
    {
        std::vector<unsigned char> reservedInfo{ 0x00, 0x02 };
        res = (FaceSdkResult)_get_codec_info(reservedInfo);
        if (FaceSdkOk == res && reservedInfo.size() >= 4)
        {
            _license_number = reservedInfo[3];
        }
    }

    return res;
}

void DecodeManager::UseOne()
{
    --_license_number;
}

void DecodeManager::ReturnOne()
{
    ++_license_number;
}

BaseDecoder* DecodeManager::OpenRTSP(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, bool async) throw(BaseException)
{
    if (_license_number <= 0)
    {
        throw BaseException(0, "OpenRTSP(" + url + ") can not be opened: license limitation");
    }

    BaseDecoder* baseDecoder(nullptr);
    if (decoderParam.codec == CODEC_CUVID)
    {
        if (async)
        {
            if (decoderParam.device_index >= 0)
            {
                baseDecoder = new AsyncGpuMatDecoder(url, decoderParam, decodeParam, id);
            }
            else
            {
                baseDecoder = new AsyncMatDecoder(url, decoderParam, decodeParam, id);
            }
        } 
        else
        {
            if (decoderParam.device_index >= 0)
            {
                baseDecoder = new GpuMatDecoder(url, decoderParam, decodeParam, id);
            }
            else
            {
                baseDecoder = new MatDecoder(url, decoderParam, decodeParam, id);
            }
        }
    }
    else if (decoderParam.codec == CODEC_NONE)
    {
        AvOptions avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
        avOptions.insert(std::make_pair("analyzeduration", "20000000"));
        avOptions.insert(std::make_pair("probesize", "20000000"));
        if (PROTOCL_TCP == decoderParam.protocol)
        {
            avOptions.insert(std::make_pair("rtsp_transport", "tcp"));
        }
        else
        {
            avOptions.insert(std::make_pair("rtsp_transport", "udp"));
        }
        baseDecoder = new Dxva2Decoder(url, decoderParam, decodeParam, id, avOptions, GetConsoleWindow());
    }
    else
    {
        throw BaseException(0, "OpenRTSP(" + url + ") can not be opened: unknown codec type(" + std::to_string(decoderParam.codec) + ")");
    }

    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }
    return baseDecoder;
}

BaseDecoder* DecodeManager::OpenVideo(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, bool async) throw(BaseException)
{
    if (_license_number <= 0)
    {
        throw BaseException(0, "OpenVideo(" + url + ") can not be opened: license limitation");
    }

    BaseDecoder* baseDecoder(nullptr);
    if (decoderParam.codec == CODEC_CUVID)
    {
        if (async)
        {
            if (decoderParam.device_index >= 0)
            {
                baseDecoder = new AsyncGpuMatDecoder(url, decoderParam, decodeParam, id, false);
            }
            else
            {
                baseDecoder = new AsyncMatDecoder(url, decoderParam, decodeParam, id, false);
            }
        } 
        else
        {
            if (decoderParam.device_index >= 0)
            {
                baseDecoder = new GpuMatDecoder(url, decoderParam, decodeParam, id);
            }
            else
            {
                baseDecoder = new MatDecoder(url, decoderParam, decodeParam, id);
            }
        }
    }
    else if (decoderParam.codec == CODEC_NONE)
    {
        AvOptions avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
        avOptions.insert(std::make_pair("analyzeduration", "20000000"));
        avOptions.insert(std::make_pair("probesize", "20000000"));
        if (PROTOCL_TCP == decoderParam.protocol)
        {
            avOptions.insert(std::make_pair("rtsp_transport", "tcp"));
        }
        else
        {
            avOptions.insert(std::make_pair("rtsp_transport", "udp"));
        }
        baseDecoder = new Dxva2Decoder(url, decoderParam, decodeParam, id, avOptions, GetConsoleWindow());
    }
    else
    {
        throw BaseException(0, "OpenVideo(" + url + ") can not be opened: unknown codec type(" + std::to_string(decoderParam.codec) + ")");
    }

    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }

    return baseDecoder;
}

BaseDecoder* DecodeManager::OpenUSB(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    if (_license_number <= 0)
    {
        throw BaseException(0, "OpenUSB(" + url + ") can not be opened: license limitation");
    }

    BaseDecoder* baseDecoder = new OpencvDecoder(url, decoderParam, decodeParam, id);
    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }
    return baseDecoder;
}

BaseDecoder* DecodeManager::OpenDirectory(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    if (_license_number <= 0)
    {
        throw BaseException(0, "OpenDirectory(" + url + ") can not be opened: license limitation");
    }

    BaseDecoder* baseDecoder = new DirectoryDecoder(url, decoderParam, decodeParam, id);
    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }
    return baseDecoder;
}

void DecodeManager::CloseDecoder(BaseDecoder* baseDecoder) throw(BaseException)
{
    if (baseDecoder)
    {
        baseDecoder->Destroy();
        ++_license_number;
        delete baseDecoder;
        baseDecoder = nullptr;
    }
}

void DecodeManager::TryCreateDecoder(BaseDecoder* baseDecoder) throw(BaseException)
{
    try
    {
        baseDecoder->Create();
        --_license_number;
    }
    catch (BaseException& ex)
    {
        baseDecoder->Destroy();
        delete baseDecoder;
        baseDecoder = nullptr;
        throw ex;
    }
    catch (...)
    {
        baseDecoder->Destroy();
        delete baseDecoder;
        baseDecoder = nullptr;
        throw BaseException(0, "unknown exception when create decoder");
    }
}


