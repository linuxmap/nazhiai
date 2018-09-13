/**
* 基于NVIDIA GPU的JPEG图像编码 & 解码
*/
#ifdef DECODER_EXPORTS
#define JPEG_CODEC_API __declspec(dllexport)
#else
#define JPEG_CODEC_API
#endif

#pragma warning(disable: 4996)
#include "opencv2/core.hpp"
#pragma warning(default: 4996)

namespace jpeg_codec
{
	/*
	* 定义改模块可能返回的错误码，其中：
	* 若返回JPEG_CODER_CUDA_ERROR附近的错误码，其实际cuda错误码为(code - JPEG_CODER_CUDA_ERROR)，
	* 参考C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v8.0\include\driver_types.h文件中的cudaError相关定义；
	*
	* 若返回JPEG_CODER_NPP_ERROR附近的错误码，其实际NPP错误码为(code - JPEG_CODER_NPP_ERROR)，
	* 参考C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v8.0\include\nppdefs.h文件中的NppStatus相关定义；
	*/
	enum
	{
		JPEG_CODER_SUCCESS = 0,
		JPEG_CODER_INVALID_GPU_INDEX,
		JPEG_CODER_NO_MEM,
		JPEG_CODER_BUFFER_TOO_SMALL,
		JPEG_CODER_INVALID_JPEG_FILE,
		JPEG_CODER_CUDA_ERROR = 0x10000000,
		JPEG_CODER_NPP_ERROR = 0x20000000,
		JPEG_CODER_MAX
	};

	struct coder_instance;
	struct decoder_instance;

	/**
	* @brief   初始化并获取一个jpeg_encode_instance句柄
	*
	* @param   instance[out]    句柄
	* @param   device_index[in]         GPU编号：0，1，2...
	* @param   max_jpeg_size           压缩后JPEG文件大小最大值
	*
	* @return  成功：0，其他：失败
	*/
	JPEG_CODEC_API int create_coder(
		coder_instance **instance,
		int device_index = 0,
		int max_jpeg_size = (4 << 20)
		);

	/**
	* @brief   输入一个cv::cuda::GpuMat, 输出对应的JPEG图像
	*
	* @param   instance[in]                   句柄
	* @param   output_buf[in][out]         输出JPEG图像的缓存
	* @param   output_buf_len[in]          输出JPEG图像的缓存大小
	* @param   valid_out_len[out]          实际输出JPEG图像大小
	* @param   mat[in]                          输入非压缩的图像
	*
	* @return  成功：0，其他：失败
	*/
	JPEG_CODEC_API int code_image(
		coder_instance *instance,
		unsigned char *output_buf,
		int output_buf_len,
		int *valid_out_len,
		const cv::cuda::GpuMat mat
		);

	/**
	* @brief   销毁句柄
	*
	* @param   instance[in]    句柄
	*
	* @return  成功：N/A
	*/
	JPEG_CODEC_API void destroy_coder(
		coder_instance *instance
		);
	/**
	* @brief   初始化并获取一个jpeg_decode_instance句柄
	*
	* @param   instance[out]    句柄
	* @param   device_index[in]         GPU编号：0，1，2...
	*
	* @return  成功：0，其他：失败
	*/
	JPEG_CODEC_API int create_decoder(
		decoder_instance **instance,
		int device_index
		);
	/**
	* @brief   输入一个JPEG图片内存，输出解码后的cv::cuda::GpuMat
	*
	* @param   instance[in]                   句柄
	* @param   input_buf[in]                 输入JPEG图像
	* @param   input_buf_len[in]          输入JPEG图像的缓存大小
	* @param   mat[in][out]                          输出解码后的图像
	*
	* @return  成功：0，其他：失败
	*/
	JPEG_CODEC_API int decode_image(
		decoder_instance *instance,
		unsigned char* input_buf,
		int input_buf_len,
		cv::cuda::GpuMat &mat
		);
	/**
	* @brief   销毁句柄
	*
	* @param   instance[in]    句柄
	*
	* @return  成功：N/A
	*/
	JPEG_CODEC_API void destroy_decoder(
		decoder_instance *instance
		);
}