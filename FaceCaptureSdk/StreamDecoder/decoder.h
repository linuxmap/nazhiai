/**
* 支持基于NVIDIA GPU的硬解码
* 支持从GPU和CPU获取解码后的图像数据
* 支持多GPU多路数据同时解码
* 支持视频压缩格式：H.264, H.265(HEVC), MPEG4, MPEG1VIDEO, MPEG2VIDEO, MJPEG, VP8, VP9, VC1
* 支持CUDA版本v8.0
*
* 同一个decoder_instance的创建，使用，销毁必须在同一个线程中进行；
* 不同的decoder_instance可以在不同的线程中使用；
* 使用不同的线程执行多个decoder_instance比使用单个线程多占用显存，多多少视具体显卡而定；
*/
#ifdef DECODER_EXPORTS
#define DECODER_API __declspec(dllexport)
#else
#define DECODER_API
#endif

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
			, max_surfaces(25)
			, synchronize(true)
		{}
	};

	template<class matT>
	struct decoder_frame
	{
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
