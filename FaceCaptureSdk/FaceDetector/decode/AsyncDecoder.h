#ifndef _CUDADECODER_HEADER_H_
#define _CUDADECODER_HEADER_H_

#include "BaseDecoder.h"
#include "decoder.h"

typedef decoder::decoder_instance<cv::Mat>* MatDecoderInstance;
typedef std::vector<MatDecoderInstance> MatDecoderInstances;

typedef decoder::decoder_instance<cv::cuda::GpuMat>* GpuMatDecoderInstance;
typedef std::vector<GpuMatDecoderInstance> GpuMatDecoderInstances;

class AsyncMatDecoder : public BaseDecoder
{
public:
    AsyncMatDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&, int async = true);
    virtual ~AsyncMatDecoder();

    void Create() throw(BaseException);
    void Destroy();

    const char* Name() const
    {
        return "AsyncMatDecoder";
    }

    bool IsAsync()
    {
        return true;
    }

protected:
    bool Init();
    void Uninit();
    bool ReadFrame(DecodedFrame& frame);

private:
    decoder::decoder_param _sdkDecoderParam;
    MatDecoderInstance _decoder; 

private:
    AsyncMatDecoder();
    AsyncMatDecoder(const AsyncMatDecoder&);
    AsyncMatDecoder& operator=(const AsyncMatDecoder&);
};

class AsyncGpuMatDecoder : public BaseDecoder
{
public:
    AsyncGpuMatDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&, int async = true);
    virtual ~AsyncGpuMatDecoder();

    void Create() throw(BaseException);
    void Destroy();

    const char* Name() const
    {
        return "AsyncGpuMatDecoder";
    }

    bool IsAsync()
    {
        return true;
    }

protected:
    bool Init();
    void Uninit();
    bool ReadFrame(DecodedFrame& frame);

private:
    decoder::decoder_param _sdkDecoderParam;
    GpuMatDecoderInstance _decoder;

private:
    AsyncGpuMatDecoder();
    AsyncGpuMatDecoder(const AsyncGpuMatDecoder&);
    AsyncGpuMatDecoder& operator=(const AsyncGpuMatDecoder&);
};

#endif

