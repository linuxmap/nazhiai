#ifndef _GPUDECODER_HEADER_H_
#define _GPUDECODER_HEADER_H_

#include "BaseDecoder.h"
#include "decoder.h"

typedef decoder::decoder_instance<cv::Mat>* MatDecoderInstance;
typedef std::vector<MatDecoderInstance> MatDecoderInstances;

typedef decoder::decoder_instance<cv::cuda::GpuMat>* GpuMatDecoderInstance;
typedef std::vector<GpuMatDecoderInstance> GpuMatDecoderInstances;

class MatDecoder : public BaseDecoder
{
public:
    MatDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~MatDecoder();

    const char* Name() const
    {
        return "MatDecoder";
    }

protected:
    bool Init();
    void Uninit();

    bool ReadFrame(DecodedFrame& frame);

private:
    decoder::decoder_param _sdkDecoderParam;
    MatDecoderInstance _decoder;

private:
    MatDecoder();
    MatDecoder(const MatDecoder&);
    MatDecoder& operator=(const MatDecoder&);
};

class GpuMatDecoder : public BaseDecoder
{
public:
    GpuMatDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~GpuMatDecoder();

    const char* Name() const
    {
        return "GpuMatDecoder";
    }

protected:
    bool Init();
    void Uninit();

    bool ReadFrame(DecodedFrame& frame);

private:
    decoder::decoder_param _sdkDecoderParam;
    GpuMatDecoderInstance _decoder;

private:
    GpuMatDecoder();
    GpuMatDecoder(const GpuMatDecoder&);
    GpuMatDecoder& operator=(const GpuMatDecoder&);
};

#endif

