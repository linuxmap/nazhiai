
#ifndef _GPUCTXINDEX_HEADER_H_
#define _GPUCTXINDEX_HEADER_H_

class GpuCtxIndex
{
private:
    static const int MAX_GPU_COUNT = 16;

public:
    static int Next(int gpuIndex = 0);

private:
    static volatile int _nextIndex[MAX_GPU_COUNT];
};

#endif

