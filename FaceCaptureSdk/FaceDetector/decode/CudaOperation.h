
#ifndef _CUDAOPERATION_HEADER_H_
#define _CUDAOPERATION_HEADER_H_

#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <curand.h>
#include <driver_types.h>  // cuda driver types
#include <device_launch_parameters.h>

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#include "opencv2/opencv.hpp"
#pragma warning(default:4819)
#pragma warning(default:4996)

class CudaOperation
{
private:
    static const int MAX_GPU_COUNT = 16;

public:
    static void Initialize();
    static void Destroy();

    static bool MemAlloc(void** ppMem, size_t size);
    static bool CopyTo(cv::cuda::GpuMat& dst, cv::cuda::GpuMat& src);
    static bool CopyTo(cv::cuda::GpuMat& dst, cv::cuda::GpuMat& src, int gpuIndex);

private:
    static cudaEvent_t cuda_events[MAX_GPU_COUNT];
    static cudaStream_t cuda_streams[MAX_GPU_COUNT];

private:
    CudaOperation();
    CudaOperation(const CudaOperation&);
    CudaOperation& operator=(const CudaOperation&);
};

#endif

