
#include "GpuDecoder.h"
#include "XMatPool.h"

#include "AutoLock.h"

#include "StreamParsor.h"
#include "CudaOperation.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#pragma comment(lib, "decoder.lib")

#define ERROR_STRING_LEN 128

MatDecoder::MatDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id), _sdkDecoderParam(), _decoder(nullptr)
{
    _sdkDecoderParam.codec = decoderParam.codec;
    _sdkDecoderParam.rtsp_protocol = decoderParam.protocol;
    _sdkDecoderParam.device_index = decoderParam.device_index;
    _sdkDecoderParam.output_width = decoderParam.width;
    _sdkDecoderParam.output_height = decoderParam.height;
    _sdkDecoderParam.synchronize = true;
    _sdkDecoderParam.max_surfaces = decoderParam.buffer_size;

    _sdkDecoderParam.log_level = decoder::LOG_LEVEL_FATAL;
}

MatDecoder::~MatDecoder()
{
    
}

bool MatDecoder::Init()
{
    AUTOLOCK(_decoderLocker);

    _nextFrameId = 0;

    StreamParsor parsor(_url);
    StreamInfo info;
    std::string err;
    if (parsor.Parse(info, err))
    {
        _origFrameInterval = info.interval;
    }
    else
    {
        if (_decodeParam.fps > 0.0f)
        {
            _origFrameInterval = 1000.0f / _decodeParam.fps;
        }
        else
        {
            _origFrameInterval = 40.0f;
        }
        LOG(WARNING) << "parse media(" << _url << ") information failed: " << err;
    }

    memset(_sdkDecoderParam.address, 0, sizeof(_sdkDecoderParam.address));
    memcpy(_sdkDecoderParam.address, _url.c_str(), _url.size());

    int ret = 0;
    int retryTimes = 5;
    char error_str[ERROR_STRING_LEN] = { '\0' };
    while ((ret = decoder::create_decoder<cv::Mat>(&_decoder, _sdkDecoderParam)) != 0
        && retryTimes-- > 0)
    {
        memset(error_str, 0, ERROR_STRING_LEN);
        decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
        LOG(INFO) << __FUNCTION__ << " create_decoder<cv::Mat>(" << _id << ") failed: " << error_str << ", try again 500ms later";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (_decoder)
    {
        _errorMessage = "";
        LOG(INFO) << Name() << "(" << _id << ") create success";
        return true;
    }
    else
    {
        _errorMessage = "MatDecoder(" + _url + ") create failed: " + error_str + "(" + std::to_string(ret) + ")";
        LOG(INFO) << _errorMessage;
        return false;
    }
}

void MatDecoder::Uninit()
{
    AUTOLOCK(_decoderLocker);

    _nextFrameId = 0;

    if (_decoder)
    {
        decoder::destroy_decoder<cv::Mat>(_decoder);
        _decoder = nullptr;
        LOG(INFO) << Name() << "(" << _id << ") destroy success";
    }
    else
    {
        LOG(INFO) << Name() << "(" << _id << ") is not created yet";
    }
}

bool MatDecoder::ReadFrame(DecodedFrame& frame)
{
    decoder::decoder_frame<cv::Mat> decoded_frame;
    int ret = decoder::retrieve_frame<cv::Mat>(_decoder, decoded_frame);
    if (ret == 0)
    {
        frame.position = (long long)(_origFrameInterval * _nextFrameId);
        _nextFrameId++;
        frame.mat = decoded_frame.mat.clone();
        frame.id = decoded_frame.frame_index;

        ret = decoder::unref_frame<cv::Mat>(_decoder, decoded_frame);
        if (ret)
        {
            char error_str[ERROR_STRING_LEN] = { '\0' };
            decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
            LOG(WARNING) << __FUNCTION__ << " unref_frame<cv::Mat> failed: " << error_str;
        }
        return true;
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

GpuMatDecoder::GpuMatDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id), _sdkDecoderParam(), _decoder(nullptr)
{
    _sdkDecoderParam.codec = decoderParam.codec;
    _sdkDecoderParam.rtsp_protocol = decoderParam.protocol;
    _sdkDecoderParam.device_index = decoderParam.device_index;
    _sdkDecoderParam.output_width = decoderParam.width;
    _sdkDecoderParam.output_height = decoderParam.height;
    _sdkDecoderParam.synchronize = true;
    _sdkDecoderParam.max_surfaces = decoderParam.buffer_size;

    _sdkDecoderParam.log_level = decoder::LOG_LEVEL_FATAL;

    _buffered = true;
}

GpuMatDecoder::~GpuMatDecoder()
{

}

bool GpuMatDecoder::Init()
{
    AUTOLOCK(_decoderLocker);

    _nextFrameId = 0;

    StreamParsor parsor(_url);
    StreamInfo info;
    std::string err;
    if (parsor.Parse(info, err))
    {
        _origFrameInterval = info.interval;
    }
    else
    {
        if (_decodeParam.fps > 0.0f)
        {
            _origFrameInterval = 1000.0f / _decodeParam.fps;
        }
        else
        {
            _origFrameInterval = 40.0f;
        }
        LOG(WARNING) << "parse media(" << _url << ") information failed: " << err;
    }

    cv::cuda::setDevice(_decoderParam.device_index);

    memset(_sdkDecoderParam.address, 0, sizeof(_sdkDecoderParam.address));
    memcpy(_sdkDecoderParam.address, _url.c_str(), _url.size());

    int ret = 0;
    int retryTimes = 5;
    char error_str[ERROR_STRING_LEN] = { '\0' };
    while ((ret = decoder::create_decoder<cv::cuda::GpuMat>(&_decoder, _sdkDecoderParam)) != 0
        && retryTimes-- > 0)
    {
        memset(error_str, 0, ERROR_STRING_LEN);
        decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
        LOG(INFO) << __FUNCTION__ << " create_decoder<cv::cuda::GpuMat>(" << _id << ") failed: " << error_str << ", try again 500ms later";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (_decoder)
    {
        _errorMessage = "";
        LOG(INFO) << Name() << "(" << _id << ") create success";
        return true;
    }
    else
    {
        _errorMessage = "GpuMatDecoder(" + _url + ") create failed: " + error_str + "(" + std::to_string(ret) + ")";
        LOG(INFO) << _errorMessage;
        return false;
    }
}

void GpuMatDecoder::Uninit()
{
    AUTOLOCK(_decoderLocker);

    _nextFrameId = 0;

    if (_decoder)
    {
        decoder::destroy_decoder<cv::cuda::GpuMat>(_decoder);
        _decoder = nullptr;
        LOG(INFO) << Name() << "(" << _id << ") destroy success";
    }
    else
    {
        LOG(INFO) << Name() << "(" << _id << ") is not created yet";
    }
}

bool GpuMatDecoder::ReadFrame(DecodedFrame& frame)
{
    decoder::decoder_frame<cv::cuda::GpuMat> decoded_frame;
    int ret = decoder::retrieve_frame<cv::cuda::GpuMat>(_decoder, decoded_frame);
    if (ret == 0)
    {
        frame.position = (long long)(_origFrameInterval * _nextFrameId);
        _nextFrameId++;

        bool copyCudaSuccess = true;
        frame.id = decoded_frame.frame_index;
        if (XMatPool<cv::cuda::GpuMat>::Alloc(decoded_frame.mat, frame.gpumat, _sdkDecoderParam.device_index))
        {
            copyCudaSuccess = CudaOperation::CopyTo(frame.gpumat, decoded_frame.mat);
        }
        else
        {
            frame.gpumat = decoded_frame.mat.clone();
        }

        ret = decoder::unref_frame<cv::cuda::GpuMat>(_decoder, decoded_frame);
        if (ret)
        {
            char error_str[ERROR_STRING_LEN] = { '\0' };
            decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
            LOG(WARNING) << __FUNCTION__ << " unref_frame<cv::cuda::GpuMat> failed: " << error_str;
        }
        return copyCudaSuccess;
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}


