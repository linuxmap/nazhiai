#ifndef _SDK_HARDWARE_DECODER_H_
#define _SDK_HARDWARE_DECODER_H_

/**
* 硬解码SDK
*
* 支持基于NVIDIA GPU的硬解码
* 支持从GPU和CPU获取解码后的图像数据
* 支持多GPU多路数据同时解码
* 支持视频压缩格式：H.264, H.265(HEVC), MPEG4, MPEG1VIDEO, MPEG2VIDEO, MJPEG, VP8, VP9, VC1
* 支持CUDA版本v8.0
*
* 使用注意事项，由于cuda环境的特殊性，为了性能和显存优化方面的考虑，请遵守以下接口实用规范：
* 1. 同一个decoder_instance的创建，使用，销毁必须在同一个线程中进行；
* 2. 不同的decoder_instance可以在不同的线程中使用；
* 3. 同一个GPU上进行的解码放在同一个线程中，放在不同的线程中会有额外的显存开销；
* 4. 不同GPU上的解码放在不同的线程中，因为一个线程只能绑定一个GPU；
* 5. 解码本地文件（不是本地文件转的RTSP流），必须实用同步模式，异步模式会有大量丢帧；
* 6. 解码RTSP流（包含本地文件转的RTSP流），必须实用异步模式，同步模式会阻塞线程的执行，影响解码的帧率；
* 7. 为了保证解码视频效果，在一般工况下，请使用RTSP_TCP模式，否则无法保证网络出现突发拥塞导致的丢包，降低图像质量；
*/
#ifdef DECODER_EXPORTS
#define DECODER_API __declspec(dllexport)
#else
#define DECODER_API
#endif

#include <vector>

namespace decoder
{
	enum
	{
		RTSP_TCP = 0,
		RTSP_UDP = 1
	};

	/** 定义支持的硬解码器类型*/
	enum
	{
		CODEC_CUVID,
		CODEC_NONE
	};

	/** 日志级别，引用FFMPEG的定义*/
	enum
	{
		LOG_LEVEL_FATAL = 8,
		LOG_LEVEL_ERROR = 16,
		LOG_LEVEL_WARNING = 24,
		LOG_LEVEL_INFO = 32,
		LOG_LEVEL_VERBOSE = 40,
		LOG_LEVEL_DEBUG = 48,
		LOG_LEVEL_TRACE = 56
	};

#define VIDEO_ADDRESS_NAME_LEN 128
	/** 硬解码初始化参数*/
	struct decoder_param
	{
		/** 码流地址，文件地址或RTSP地址*/
		char address[VIDEO_ADDRESS_NAME_LEN];
		/** 选择解码器，CODEC_XXX*/
		int codec;
		/** 当codec == CODEC_CUVID时，指定使用的GPU编号：0,1,2...*/
		int device_index;
		/** RTSP_XXX*/
		int rtsp_protocol;
		/** 输出图像宽高，如果有一个为0，则使用原始size进行输出*/
		int output_width;
		int output_height;
		/** 日志级别，LOG_LEVEL_XXX，输出不同级别的FFMPEG日志*/
		int log_level;
		/** 解码缓存队列长度*/
		int max_surfaces;
		/**
		* true: 同步模式，调用retrieve_next_frame时阻塞当前线程，直到获取到下一帧数据或出错；
		* false: 异步模式，调用retrieve_next_frame时不阻塞当前线程，如果获取不到数据，直接返回错误码；
		* 在同一个线程里面解码多路RTSP实时视频流时，建议使用异步模式；同步模式下同一个
		* 线程里面解码的视频流的帧率以最低的为准；
		* 在解码视频文件的时候，建议使用同步模式，以保证不丢帧；
		*/
		bool synchronize;
		decoder_param()
			: rtsp_protocol(RTSP_TCP)
			, codec(CODEC_NONE)
			, device_index(0)
			, output_width(0)
			, output_height(0)
			, log_level(LOG_LEVEL_ERROR)
			, max_surfaces(100)
			, synchronize(true)
		{}
	};

	template<class matT>
	struct decoder_frame
	{
		unsigned long long frame_index;
		matT mat;
	};

	template<class matT>
	class decoder_instance;

	/**
	* @brief   初始化并获取一个decoder_instance句柄
	*
	* @param   instance[out]    句柄
	* @param   decoder_param[in]         初始化参数
	*
	* @return  成功：0，其他：失败
	*/
	template<class matT>
	int create_decoder(
		decoder_instance<matT> **instance,
		const decoder_param &param
		);

	/**
	* @brief   销毁句柄
	*
	* @param   instance[in]    句柄
	*
	* @return  成功：0，其他：失败
	*/
	template<class matT>
	int destroy_decoder(
		decoder_instance<matT> *instance
		);

	/**
	* @brief   获取一帧解码后的数据
	*
	* @param   instance[in]    句柄
	* @param   frame[in out]        输出数据
	*
	* @return  成功：0，其他：失败
	*/
	template<class matT>
	int retrieve_frame(
		decoder_instance<matT> *instance,
		decoder_frame<matT> &frame);

	/**
	* @brief   回收数据占用的内存/显存，由retrieve_next_frame获取到的frame必须由该接口进行释放；
	*               如果有超过max_surfaces个frame没有被释放，会申请不到解码的资源，解码失败；
	*
	* @param   instance[in]    句柄
	* @param   frame[in]        需要释放的frame
	*
	* @return  成功：0，其他：失败
	*/
	template<class matT>
	int unref_frame(
		decoder_instance<matT> *instance,
		decoder_frame<matT> &frame
		);

	/**
	* @brief   获取错误码的描述
	*
	* @param   buffer[in out]    预先申请好的缓存，存放描述内容
	* @param   buffer_size[in]        缓存大小
	* @param   error[in]        错误码
	*
	* @return  buffer
	*/
	DECODER_API
	const char* decoder_error_string(
		char *buffer,
		size_t buffer_size,
		int error
		);

	/**
	* @brief   获取版本号
	*/
	DECODER_API
	void decoder_version(
		int &major,
		int &minor
		);
}

DECODER_API int _get_codec_info(std::vector<unsigned char> &info);

#endif

