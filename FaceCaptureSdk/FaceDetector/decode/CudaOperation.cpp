#include "CudaOperation.h"

#include "FaceSdk.h"

#pragma comment(lib, "cudart.lib")

cudaEvent_t CudaOperation::cuda_events[MAX_GPU_COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

cudaStream_t CudaOperation::cuda_streams[MAX_GPU_COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

void CudaOperation::Initialize()
{
    int gpuCount = 0;
    if (FaceSdkOk == GetGpuCount(gpuCount) && gpuCount > 0)
    {
        for (int idx = 0; idx < gpuCount && idx < MAX_GPU_COUNT; ++idx)
        {
            cudaEventCreate(&cuda_events[idx]);
            cudaStreamCreate(&cuda_streams[idx]);
        }
    }
}

void CudaOperation::Destroy()
{
    for (int idx = 0; idx < MAX_GPU_COUNT; ++idx)
    {
        if (cuda_streams[idx])
        {
            cudaEventDestroy(cuda_events[idx]);
            cuda_events[idx] = nullptr;

            cudaStreamDestroy(cuda_streams[idx]);
            cuda_streams[idx] = nullptr;
        }
    }
}

bool CudaOperation::MemAlloc(void** ppMem, size_t size)
{
    return cudaSuccess == cudaMalloc(ppMem, size) && *ppMem;
}

bool CudaOperation::CopyTo(cv::cuda::GpuMat& dst, cv::cuda::GpuMat& src)
{
    return cudaSuccess == cudaMemcpy(dst.data, src.data, src.step*src.rows, cudaMemcpyDeviceToDevice);
}

bool CudaOperation::CopyTo(cv::cuda::GpuMat& dst, cv::cuda::GpuMat& src, int gpuIndex)
{
    if (gpuIndex >= MAX_GPU_COUNT || gpuIndex < 0 || !cuda_streams[gpuIndex] || !cuda_events[gpuIndex])
    {
        return false;
    }

    if (cudaSuccess == cudaMemcpyAsync(dst.data, src.data, src.step*src.rows, cudaMemcpyDeviceToDevice, cuda_streams[gpuIndex]))
    {
        if (cudaSuccess == cudaStreamSynchronize(cuda_streams[gpuIndex]))
        {
            return true;
        }
    }
    return false;
}



