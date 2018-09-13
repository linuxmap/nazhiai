#include "GpuCtxIndex.h"


int GpuCtxIndex::Next(int gpuIndex)
{
    return _nextIndex[gpuIndex]++;
}

volatile int GpuCtxIndex::_nextIndex[MAX_GPU_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

