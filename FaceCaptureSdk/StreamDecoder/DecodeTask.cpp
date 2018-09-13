#include "DecodeTask.h"

#include "DirectoryDecoder.h"
#include "OpencvDecoder.h"
#include "Dxva2Decoder.h"
#include "GpuDecoder.h"

BaseDecoder* DecodeTask::OpenRTSP(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    BaseDecoder* baseDecoder(nullptr);
    if (decoderParam.codec == CODEC_CUVID)
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
    else if (decoderParam.codec == CODEC_NONE)
    {
        AvOptions avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
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

void DecodeTask::CloseRTSP(BaseDecoder* baseDecoder) throw(BaseException)
{
    if (baseDecoder)
    {
        TryDestoryDecoder(baseDecoder);
    }
}

BaseDecoder* DecodeTask::OpenVideo(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    BaseDecoder* baseDecoder(nullptr);
    if (decoderParam.codec == CODEC_CUVID)
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
    else if (decoderParam.codec == CODEC_NONE)
    {
        AvOptions avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
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

void DecodeTask::CloseVideo(BaseDecoder* baseDecoder) throw(BaseException)
{
    if (baseDecoder)
    {
        TryDestoryDecoder(baseDecoder);
    }
}

BaseDecoder* DecodeTask::OpenUSB(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    BaseDecoder* baseDecoder = new OpencvDecoder(url, decoderParam, decodeParam, id);
    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }
    return baseDecoder;
}

void DecodeTask::CloseUSB(BaseDecoder* baseDecoder) throw(BaseException)
{
    if (baseDecoder)
    {
        TryDestoryDecoder(baseDecoder);
    }
}

BaseDecoder* DecodeTask::OpenDirectory(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id) throw(BaseException)
{
    BaseDecoder* baseDecoder = new DirectoryDecoder(url, decoderParam, decodeParam, id);
    if (baseDecoder)
    {
        TryCreateDecoder(baseDecoder);
    }
    return baseDecoder;
}

void DecodeTask::CloseDirectory(BaseDecoder* baseDecoder) throw(BaseException)
{
    if (baseDecoder)
    {
        TryDestoryDecoder(baseDecoder);
    }
}

DecodeTask* DecodeTask::CreateTask()
{
    return new DecodeTask();
}

void DecodeTask::DestroyTask(DecodeTask* decodeTask)
{
    if (decodeTask)
    {
        decodeTask->ClearDecoders();
        delete decodeTask;
    }
}

bool DecodeTask::GetFrames(std::vector<DecodedFramePtr>& decodedFramePtrs)
{
    AUTOLOCK(_locker);
    for (std::vector<BaseDecoder*>::iterator it = _decoders.begin(); it != _decoders.end(); ++it)
    {
        BaseDecoder* baseDecoder = *it;
        DecodedFramePtr decodedFramePtr = baseDecoder->GetFrame();
        if (decodedFramePtr)
        {
            decodedFramePtrs.push_back(decodedFramePtr);
        }
    }
    return decodedFramePtrs.size() > 0;
}

DecodeTask::DecodeTask()
    : _locker(), _decoders()
{

}

void DecodeTask::AddDecoder(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        AUTOLOCK(_locker);
        _decoders.push_back(baseDecoder);
    }
}

void DecodeTask::DelDecoder(BaseDecoder* baseDecoder)
{
    if (baseDecoder)
    {
        AUTOLOCK(_locker);
        for (std::vector<BaseDecoder*>::iterator it = _decoders.begin(); it != _decoders.end(); ++it)
        {
            BaseDecoder* baseDecoder = *it;
            if (baseDecoder == baseDecoder)
            {
                if (baseDecoder)
                {
                    DestoryDecoder(baseDecoder);
                }
                _decoders.erase(it);
                break;
            }
        }
    }
}

void DecodeTask::ClearDecoders()
{
    AUTOLOCK(_locker);
    for (std::vector<BaseDecoder*>::iterator it = _decoders.begin(); it != _decoders.end(); ++it)
    {
        BaseDecoder* baseDecoder = *it;
        if (baseDecoder)
        {
            DestoryDecoder(baseDecoder);
        }
    }
    _decoders.clear();
}

void DecodeTask::TryCreateDecoder(BaseDecoder* baseDecoder) throw(BaseException)
{
    try
    {
        baseDecoder->Create();
    }
    catch (BaseException& ex)
    {
        delete baseDecoder;
        baseDecoder = nullptr;
        throw ex;
    }
    catch (...)
    {
        delete baseDecoder;
        baseDecoder = nullptr;
        throw BaseException(0, "Unknown exception when CreateRTSP");
    }
}

void DecodeTask::TryDestoryDecoder(BaseDecoder* baseDecoder) throw(BaseException)
{
    try
    {
        baseDecoder->Destroy();
    }
    catch (BaseException& ex)
    {
        delete baseDecoder;
        baseDecoder = nullptr;
        throw ex;
    }
    catch (...)
    {
        delete baseDecoder;
        baseDecoder = nullptr;
        throw BaseException(0, "Unknown exception when CreateRTSP");
    }
}

void DecodeTask::DestoryDecoder(BaseDecoder* baseDecoder) throw()
{
    try
    {
        baseDecoder->Destroy();
    }
    catch (BaseException& ex)
    {
        delete baseDecoder;
        baseDecoder = nullptr;
        printf("%s with error: %s\n", __FUNCTION__, ex.Description().c_str());
    }
    catch (...)
    {
        delete baseDecoder;
        baseDecoder = nullptr;

        printf("%s with unknown exception\n", __FUNCTION__); 
    }
}

