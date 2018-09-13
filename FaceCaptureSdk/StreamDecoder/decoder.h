/**
* ֧�ֻ���NVIDIA GPU��Ӳ����
* ֧�ִ�GPU��CPU��ȡ������ͼ������
* ֧�ֶ�GPU��·����ͬʱ����
* ֧����Ƶѹ����ʽ��H.264, H.265(HEVC), MPEG4, MPEG1VIDEO, MPEG2VIDEO, MJPEG, VP8, VP9, VC1
* ֧��CUDA�汾v8.0
*
* ͬһ��decoder_instance�Ĵ�����ʹ�ã����ٱ�����ͬһ���߳��н��У�
* ��ͬ��decoder_instance�����ڲ�ͬ���߳���ʹ�ã�
* ʹ�ò�ͬ���߳�ִ�ж��decoder_instance��ʹ�õ����̶߳�ռ���Դ棬������Ӿ����Կ�������
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

	/** ����֧�ֵ�Ӳ����������*/
	enum
	{
		CODEC_CUVID,
		CODEC_NONE
	};

	/** ��־��������FFMPEG�Ķ���*/
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
	/** Ӳ�����ʼ������*/
	struct decoder_param
	{
		/** ������ַ���ļ���ַ��RTSP��ַ*/
		char address[VIDEO_ADDRESS_NAME_LEN];
		/** ѡ���������CODEC_XXX*/
		int codec;
		/** ��codec == CODEC_CUVIDʱ��ָ��ʹ�õ�GPU��ţ�0,1,2...*/
		int device_index;
		/** RTSP_XXX*/
		int rtsp_protocol;
		/** ���ͼ���ߣ������һ��Ϊ0����ʹ��ԭʼsize�������*/
		int output_width;
		int output_height;
		/** ��־����LOG_LEVEL_XXX�������ͬ�����FFMPEG��־*/
		int log_level;
		/** ���뻺����г���*/
		int max_surfaces;
		/**
		* true: ͬ��ģʽ������retrieve_next_frameʱ������ǰ�̣߳�ֱ����ȡ����һ֡���ݻ����
		* false: �첽ģʽ������retrieve_next_frameʱ��������ǰ�̣߳������ȡ�������ݣ�ֱ�ӷ��ش����룻
		* ��ͬһ���߳���������·RTSPʵʱ��Ƶ��ʱ������ʹ���첽ģʽ��ͬ��ģʽ��ͬһ��
		* �߳�����������Ƶ����֡������͵�Ϊ׼��
		* �ڽ�����Ƶ�ļ���ʱ�򣬽���ʹ��ͬ��ģʽ���Ա�֤����֡��
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
	* @brief   ��ʼ������ȡһ��decoder_instance���
	*
	* @param   instance[out]    ���
	* @param   decoder_param[in]         ��ʼ������
	*
	* @return  �ɹ���0��������ʧ��
	*/
	template<class matT>
	int create_decoder(
		decoder_instance<matT> **instance,
		const decoder_param &param
		);

	/**
	* @brief   ���پ��
	*
	* @param   instance[in]    ���
	*
	* @return  �ɹ���0��������ʧ��
	*/
	template<class matT>
	int destroy_decoder(
		decoder_instance<matT> *instance
		);

	/**
	* @brief   ��ȡһ֡����������
	*
	* @param   instance[in]    ���
	* @param   frame[in out]        �������
	*
	* @return  �ɹ���0��������ʧ��
	*/
	template<class matT>
	int retrieve_frame(
		decoder_instance<matT> *instance,
		decoder_frame<matT> &frame);

	/**
	* @brief   ��������ռ�õ��ڴ�/�Դ棬��retrieve_next_frame��ȡ����frame�����ɸýӿڽ����ͷţ�
	*               ����г���max_surfaces��frameû�б��ͷţ������벻���������Դ������ʧ�ܣ�
	*
	* @param   instance[in]    ���
	* @param   frame[in]        ��Ҫ�ͷŵ�frame
	*
	* @return  �ɹ���0��������ʧ��
	*/
	template<class matT>
	int unref_frame(
		decoder_instance<matT> *instance,
		decoder_frame<matT> &frame
		);

	/**
	* @brief   ��ȡ�����������
	*
	* @param   buffer[in out]    Ԥ������õĻ��棬�����������
	* @param   buffer_size[in]        �����С
	* @param   error[in]        ������
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
	* @brief   ��ȡ�汾��
	*/
	DECODER_API
	void decoder_version(
		int &major,
		int &minor
		);
}
