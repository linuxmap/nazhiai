#ifndef _DECODECAPTURE_HEADER_H_
#define _DECODECAPTURE_HEADER_H_

#include "BaseDecoder.h"
#include "decoder.h"

#pragma warning(disable: 4996)
#include "opencv2/opencv.hpp"
#pragma warning(default: 4996)
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudawarping.hpp"

typedef decoder::decoder_instance<cv::Mat>* MatDecoderInstance;
typedef std::vector<MatDecoderInstance> MatDecoderInstances;

typedef decoder::decoder_instance<cv::cuda::GpuMat>* GpuMatDecoderInstance;
typedef std::vector<GpuMatDecoderInstance> GpuMatDecoderInstances;

class MatDecoder : public BaseDecoder
{
public:
    MatDecoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&);
    virtual ~MatDecoder();

    void Create() throw(BaseException);
    void Destroy() throw(BaseException);

    inline const char* Name() const
    {
        return "MatDecoder_";
    }

protected:
    bool ReadFrame(cv::Mat& frame);
    bool ReadFrame(cv::cuda::GpuMat& frame);

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

    void Create() throw(BaseException);
    void Destroy() throw(BaseException);

    inline const char* Name() const
    {
        return "MatDecoder_";
    }

protected:
    bool ReadFrame(cv::Mat& frame);
    bool ReadFrame(cv::cuda::GpuMat& frame);

private:
    decoder::decoder_param _sdkDecoderParam;
    GpuMatDecoderInstance _decoder;

private:
    GpuMatDecoder();
    GpuMatDecoder(const GpuMatDecoder&);
    GpuMatDecoder& operator=(const GpuMatDecoder&);
};

#endif

