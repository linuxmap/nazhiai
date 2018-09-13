
#include "GpuDecoder.h"
#include "XMatPool.h"

#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <curand.h>
#include <driver_types.h>  // cuda driver types
#include <device_launch_parameters.h>

#pragma comment(lib, "decoder.lib")
#pragma comment(lib, "cudart.lib")

#define ERROR_STRING_LEN 128

MatDecoder::MatDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id), _sdkDecoderParam(), _decoder()
{
    _sdkDecoderParam.codec = decoderParam.codec;
    _sdkDecoderParam.rtsp_protocol = decoderParam.protocol;
    _sdkDecoderParam.device_index = decoderParam.device_index;
    _sdkDecoderParam.output_width = decoderParam.width;
    _sdkDecoderParam.output_height = decoderParam.height;
    _sdkDecoderParam.synchronize = decoderParam.synchronize;
    _sdkDecoderParam.max_surfaces = decoderParam.buffer_size;

    _sdkDecoderParam.log_level = decoder::LOG_LEVEL_FATAL;
}

MatDecoder::~MatDecoder()
{
    
}

void MatDecoder::Create() throw(BaseException)
{
    memset(_sdkDecoderParam.address, 0, sizeof(_sdkDecoderParam.address));
    memcpy(_sdkDecoderParam.address, _url.c_str(), _url.size());

    int ret = 0;
    int retryTimes = 5;
    while ((ret = decoder::create_decoder<cv::Mat>(&_decoder, _sdkDecoderParam)) != 0
        && retryTimes-- > 0)
    {
        char error_str[ERROR_STRING_LEN] = { '\0' };
        decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
        printf("create_decoder<Mat>(%s) failed: %s, try again 500ms later\n", _url.c_str(), error_str);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (ret != 0)
    {
        throw BaseException(0, "MatDecoder(" + _url + ") can not be opened");
    }
}

void MatDecoder::Destroy() throw(BaseException)
{
    if (_decoder)
    {
        decoder::destroy_decoder<cv::Mat>(_decoder);
    }
}

bool MatDecoder::ReadFrame(cv::Mat& frame)
{
    decoder::decoder_frame<cv::Mat> decoded_frame;
    int ret = decoder::retrieve_frame<cv::Mat>(_decoder, decoded_frame);
    if (ret == 0)
    {
        if (!XMatPool<cv::Mat>::Alloc(decoded_frame.mat, frame))
        {
            frame = decoded_frame.mat.clone();
        }
        else
        {
            memcpy(frame.data, decoded_frame.mat.data, decoded_frame.mat.cols*decoded_frame.mat.rows*decoded_frame.mat.channels());
        }

        ret = decoder::unref_frame<cv::Mat>(_decoder, decoded_frame);
        if (ret)
        {
            char error_str[ERROR_STRING_LEN] = { '\0' };
            decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
            printf("unref_frame<Mat> failed: %s\n", error_str);
        }
        return !frame.empty();
    }
    return false;
}

bool MatDecoder::ReadFrame(cv::cuda::GpuMat& frame)
{
    cv::Mat mat;
    if (ReadFrame(mat))
    {
        frame.upload(mat);
        return true;
    }
    else
    {
        return false;
    }
}

GpuMatDecoder::GpuMatDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id), _sdkDecoderParam(), _decoder()
{
    _sdkDecoderParam.codec = decoderParam.codec;
    _sdkDecoderParam.rtsp_protocol = decoderParam.protocol;
    _sdkDecoderParam.device_index = decoderParam.device_index;
    _sdkDecoderParam.output_width = decoderParam.width;
    _sdkDecoderParam.output_height = decoderParam.height;
    _sdkDecoderParam.synchronize = decoderParam.synchronize;
    _sdkDecoderParam.max_surfaces = decoderParam.buffer_size;

    _sdkDecoderParam.log_level = decoder::LOG_LEVEL_FATAL;
}

GpuMatDecoder::~GpuMatDecoder()
{

}

void GpuMatDecoder::Create() throw(BaseException)
{
    memset(_sdkDecoderParam.address, 0, sizeof(_sdkDecoderParam.address));
    memcpy(_sdkDecoderParam.address, _url.c_str(), _url.size());

    int ret = 0;
    int retryTimes = 5;
    while ((ret = decoder::create_decoder<cv::cuda::GpuMat>(&_decoder, _sdkDecoderParam)) != 0
        && retryTimes-- > 0)
    {
        char error_str[ERROR_STRING_LEN] = { '\0' };
        decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
        printf("create_decoder<cv::cuda::GpuMat>(%s) failed: %s, try again 500ms later\n", _url.c_str(), error_str);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (ret != 0)
    {
        throw BaseException(0, "GpuMatDecoder(" + _url + ") can not be opened");
    }
}

void GpuMatDecoder::Destroy() throw(BaseException)
{
    if (_decoder)
    {
        decoder::destroy_decoder<cv::cuda::GpuMat>(_decoder);
    }
}

bool GpuMatDecoder::ReadFrame(cv::Mat& frame)
{
    cv::cuda::GpuMat gpumat;
    if (ReadFrame(gpumat))
    {
        gpumat.download(frame);
        return true;
    }
    else
    {
        return false;
    }
}

bool GpuMatDecoder::ReadFrame(cv::cuda::GpuMat& frame)
{
    decoder::decoder_frame<cv::cuda::GpuMat> decoded_frame;
    int ret = decoder::retrieve_frame<cv::cuda::GpuMat>(_decoder, decoded_frame);
    if (ret == 0)
    {
        if (!XMatPool<cv::cuda::GpuMat>::Alloc(decoded_frame.mat, frame, _sdkDecoderParam.device_index))
        {
            frame = decoded_frame.mat.clone();
        }
        else
        {
            cudaError cudaResult = cudaMemcpy(frame.data, decoded_frame.mat.data, decoded_frame.mat.step*decoded_frame.mat.rows, cudaMemcpyDeviceToDevice);
            if (cudaSuccess != cudaResult)
            {
                XMatPool<cv::cuda::GpuMat>::Free(frame, _sdkDecoderParam.device_index);
                printf("%s cudaMemcpy failed: %d\n", __FUNCTION__, cudaResult);
            }
        }

        ret = decoder::unref_frame<cv::cuda::GpuMat>(_decoder, decoded_frame);
        if (ret)
        {
            char error_str[ERROR_STRING_LEN] = { '\0' };
            decoder::decoder_error_string(error_str, ERROR_STRING_LEN, ret);
            printf("unref_frame<cv::cuda::GpuMat> failed: %s\n", error_str);
        }
        return !frame.empty();
    }
    return false;
}


