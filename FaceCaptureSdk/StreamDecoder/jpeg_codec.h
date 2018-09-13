/**
* ����NVIDIA GPU��JPEGͼ����� & ����
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
	* �����ģ����ܷ��صĴ����룬���У�
	* ������JPEG_CODER_CUDA_ERROR�����Ĵ����룬��ʵ��cuda������Ϊ(code - JPEG_CODER_CUDA_ERROR)��
	* �ο�C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v8.0\include\driver_types.h�ļ��е�cudaError��ض��壻
	*
	* ������JPEG_CODER_NPP_ERROR�����Ĵ����룬��ʵ��NPP������Ϊ(code - JPEG_CODER_NPP_ERROR)��
	* �ο�C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v8.0\include\nppdefs.h�ļ��е�NppStatus��ض��壻
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
	* @brief   ��ʼ������ȡһ��jpeg_encode_instance���
	*
	* @param   instance[out]    ���
	* @param   device_index[in]         GPU��ţ�0��1��2...
	* @param   max_jpeg_size           ѹ����JPEG�ļ���С���ֵ
	*
	* @return  �ɹ���0��������ʧ��
	*/
	JPEG_CODEC_API int create_coder(
		coder_instance **instance,
		int device_index = 0,
		int max_jpeg_size = (4 << 20)
		);

	/**
	* @brief   ����һ��cv::cuda::GpuMat, �����Ӧ��JPEGͼ��
	*
	* @param   instance[in]                   ���
	* @param   output_buf[in][out]         ���JPEGͼ��Ļ���
	* @param   output_buf_len[in]          ���JPEGͼ��Ļ����С
	* @param   valid_out_len[out]          ʵ�����JPEGͼ���С
	* @param   mat[in]                          �����ѹ����ͼ��
	*
	* @return  �ɹ���0��������ʧ��
	*/
	JPEG_CODEC_API int code_image(
		coder_instance *instance,
		unsigned char *output_buf,
		int output_buf_len,
		int *valid_out_len,
		const cv::cuda::GpuMat mat
		);

	/**
	* @brief   ���پ��
	*
	* @param   instance[in]    ���
	*
	* @return  �ɹ���N/A
	*/
	JPEG_CODEC_API void destroy_coder(
		coder_instance *instance
		);
	/**
	* @brief   ��ʼ������ȡһ��jpeg_decode_instance���
	*
	* @param   instance[out]    ���
	* @param   device_index[in]         GPU��ţ�0��1��2...
	*
	* @return  �ɹ���0��������ʧ��
	*/
	JPEG_CODEC_API int create_decoder(
		decoder_instance **instance,
		int device_index
		);
	/**
	* @brief   ����һ��JPEGͼƬ�ڴ棬���������cv::cuda::GpuMat
	*
	* @param   instance[in]                   ���
	* @param   input_buf[in]                 ����JPEGͼ��
	* @param   input_buf_len[in]          ����JPEGͼ��Ļ����С
	* @param   mat[in][out]                          ���������ͼ��
	*
	* @return  �ɹ���0��������ʧ��
	*/
	JPEG_CODEC_API int decode_image(
		decoder_instance *instance,
		unsigned char* input_buf,
		int input_buf_len,
		cv::cuda::GpuMat &mat
		);
	/**
	* @brief   ���پ��
	*
	* @param   instance[in]    ���
	*
	* @return  �ɹ���N/A
	*/
	JPEG_CODEC_API void destroy_decoder(
		decoder_instance *instance
		);
}