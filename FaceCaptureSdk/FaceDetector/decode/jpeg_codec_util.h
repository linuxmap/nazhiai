
#ifndef _JPEG_CODER_UTIL_HEADER_H_
#define _JPEG_CODER_UTIL_HEADER_H_

#include "jpeg_codec.h"

typedef struct jpeg_codec::coder_instance jpeg_instance;

template<int DeviceCount>
class JPegCodec
{
public:
    static void Init(int deviceIndex, int maxSize)
    {
        if (deviceIndex < DeviceCount)
        {
            if (jpeg_codec::create_coder(&device_coder_instance[deviceIndex], deviceIndex, maxSize) == 0)
            {
                jpeg_buffer[deviceIndex] = (unsigned char*)malloc(maxSize);
                jpeg_buffer_size[deviceIndex] = maxSize;
            }
        }
    }

    static int Encode(int deviceIndex, cv::cuda::GpuMat& gpuMat, unsigned char** jpeg, int& len)
    {
        int result = -1;
        if (device_coder_instance[deviceIndex] != nullptr && jpeg_buffer[deviceIndex] != nullptr)
        {
            result = jpeg_codec::code_image(device_coder_instance[deviceIndex], jpeg_buffer[deviceIndex], jpeg_buffer_size[deviceIndex], &len, gpuMat);
            *jpeg = jpeg_buffer[deviceIndex];
        }
        return result;
    }

    static int Download(int deviceIndex, cv::cuda::GpuMat& gpuMat, cv::Mat& mat)
    {
        int len = 0;
        unsigned char* jpeg = nullptr;
        int result = Encode(deviceIndex, gpuMat, &jpeg, len);
        if (0 == result && jpeg)
        {
            mat = cv::imdecode(std::vector<unsigned char>(jpeg, jpeg + len), cv::IMREAD_UNCHANGED);
        }
        return result;
    }

    static void Destroy(int deviceIndex)
    {
        if (deviceIndex < DeviceCount)
        {
            jpeg_codec::destroy_coder(device_coder_instance[deviceIndex]);
            free(jpeg_buffer[deviceIndex]);
            jpeg_buffer[deviceIndex] = nullptr;
        }
    }

private:
    static jpeg_instance* device_coder_instance[DeviceCount];
    static unsigned char* jpeg_buffer[DeviceCount];
    static int jpeg_buffer_size[DeviceCount];

private:
    JPegCodec();
    JPegCodec(const JPegCodec&);
    JPegCodec& operator=(const JPegCodec&);
};

template<int DeviceCount /*= 16*/>
jpeg_instance* JPegCodec<DeviceCount>::device_coder_instance[DeviceCount] = { nullptr };

template<int DeviceCount /*= 16*/>
unsigned char* JPegCodec<DeviceCount>::jpeg_buffer[DeviceCount] = { nullptr };

template<int DeviceCount /*= 16*/>
int JPegCodec<DeviceCount>::jpeg_buffer_size[DeviceCount] = { 0 };


#endif

