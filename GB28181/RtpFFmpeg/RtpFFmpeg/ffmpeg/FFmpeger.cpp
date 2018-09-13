
#include "FFmpeger.h"

#pragma warning(disable:4996)

FFmpeger* FFmpeger::CreateFFmpeger(char payloadType, const std::string& source)
{
    FFmpeger* ffmpeger = nullptr;
    if (96 == payloadType)
    {
        ffmpeger = new FFmpeger(AV_CODEC_ID_H264, source);
    }
    else if (97 == payloadType)
    {
        ffmpeger = new FFmpeger(AV_CODEC_ID_MPEG4, source);
    }
    else
    {
        ffmpeger = new FFmpeger(AV_CODEC_ID_H264, source);
    }
    return ffmpeger;
}

void FFmpeger::DestroyFFmpeger(FFmpeger* ffmpeger)
{
    if (ffmpeger)
    {
        delete ffmpeger;
        ffmpeger = nullptr;
    }
}

int FFmpeger::AVIORead(void* opaque, unsigned char* buf, int size)
{
    FFmpeger* ffmpeger = (FFmpeger*)opaque;
    ffmpeger->GotFrame(buf, &size);
    return size;
}

FFmpeger::AVInitor FFmpeger::av_registeror;

FFmpeger::FFmpeger(enum AVCodecID codecId, const std::string& source)
    : _codecId(codecId), _source(source)
    , _avformat(nullptr), _avio(nullptr), _aviobuffer(nullptr), _video_index(-1)
    , _origin_data(nullptr), _left_data(nullptr), _left_size(0)
    , _codec(nullptr), _codec_context(nullptr)
    , _av_packet(nullptr)
    , _frame(nullptr), _sws_context(nullptr), _frame_bgr(nullptr)
    , _pixel_format(AV_PIX_FMT_NONE)
    , _device_ctx(nullptr)
    , _mat_buffer(nullptr), _mat_buffer_size(0)
    , _decode(), _locker(), _condition(), _decoding(false)
    , _framenode_que()
{
    _decoding = true;
    _decode = std::thread(&FFmpeger::DecodeMain, this);
}

FFmpeger::~FFmpeger()
{
    _decoding = false;
    _condition.notify_one();
    if (_decode.joinable())
    {
        _decode.join();
    }
}

void FFmpeger::FeedFrame(unsigned char* packet, size_t size)
{
    if (packet && size > 0)
    {
        unsigned char* data = (unsigned char*)malloc(size);
        if (data)
        {
            memcpy(data, packet, size);

            std::lock_guard<std::mutex> lg(_locker);
            _framenode_que.push(framenode{ data, size});
            _condition.notify_one();
        }
    }
}

static void save(unsigned char* data, size_t size)
{
    static FILE* file = nullptr;
    static int packet_count = 0;
    if (!file && packet_count < 100)
    {
        file = fopen("autosave.bin", "w");
    }

    if (file)
    {
        fwrite(data, size, 1, file);
        fflush(file);

        if (++packet_count >= 1000)
        {
            fclose(file);
            file = nullptr;
        }
    }
}

void FFmpeger::GotFrame(unsigned char* frame, int* size)
{
    if (_left_data)
    {
        if (*size >= _left_size)
        {
            *size = _left_size;
            memcpy(frame, _left_data, *size);

            free(_origin_data);
            _origin_data = nullptr;
            _left_data = nullptr;
        } 
        else
        {
            memcpy(frame, _left_data, *size);
            _left_data += *size;
            _left_size -= *size;
        }
    } 
    else
    {
        framenode node{nullptr, 0};
        {
            std::unique_lock<std::mutex> ul(_locker);
            _condition.wait_for(ul, std::chrono::milliseconds(500), [this]() { return !_framenode_que.empty() || !_decoding; });
            node = _framenode_que.front();
            _framenode_que.pop();
        }

        if (!node.data)
        {
            *size = -1;
        }
        else
        {
            _left_data = node.data;
            _left_size = (int)node.size;

            //save(node.data, node.size);

            if (*size >= _left_size)
            {
                *size = _left_size;
                memcpy(frame, _left_data, *size);

                free(node.data);
                _left_data = nullptr;
                _origin_data = nullptr;
            }
            else
            {
                memcpy(frame, _left_data, *size);
                _left_data += *size;
                _left_size -= *size;
                _origin_data = node.data;
            }
        }
    }
}

bool FFmpeger::Init(AVCodecID codecid, std::string& err)
{
    enum AVHWDeviceType devType = av_hwdevice_find_type_by_name("dxva2"); 
    if (AV_HWDEVICE_TYPE_NONE != devType)
    {
        _pixel_format = ChoosePixelFormat(devType);
    }

    _codec = avcodec_find_decoder(codecid);
    if (!_codec)
    {
        err = "avcodec_find_decoder failed";
        return false;
    }

    _codec_context = avcodec_alloc_context3(_codec);
    if (!_codec_context)
    {
        err = "avcodec_alloc_context3 failed";
        return false;
    }
    
    if (AV_PIX_FMT_NONE != _pixel_format)
    {
        _codec_context->opaque = (void*)&_pixel_format;
        _codec_context->get_format = get_hw_format;
        if (DeviceInit(_codec_context, devType) < 0)
        {
            err = "device initialize failed";
            return false;
        }
    }
    
    _codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if (avcodec_open2(_codec_context, _codec, NULL) < 0)
    {
        err = "avcodec_open2 failed";
        return false;
    }

    _av_packet = av_packet_alloc();

    _frame = av_frame_alloc();
    _sw_frame = av_frame_alloc();
    _frame_bgr = av_frame_alloc();

    _avformat = avformat_alloc_context();

    _aviobuffer = (unsigned char *)av_malloc(1024);
    _avio = avio_alloc_context(_aviobuffer, 1024, 0, this, FFmpeger::AVIORead, nullptr, nullptr);

    AVInputFormat* piFmt = nullptr;
    av_probe_input_buffer(_avio, &piFmt, "", nullptr, 0, 0);

    _avformat->pb = _avio;
    if (avformat_open_input(&_avformat, nullptr, piFmt, nullptr) < 0)
    {
        err = "avformat_open_input failed";
        return false;
    }
    
    if (avformat_find_stream_info(_avformat, nullptr) < 0)
    {
        err = "avformat_find_stream_info failed";
        return false;
    }

    for (int i = 0; i < (int)_avformat->nb_streams; ++i)
    {
        if (_avformat->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _video_index = i;
        }
    }

    return true;
}

void FFmpeger::Uninit()
{
    if (_mat_buffer)
    {
        av_free(_mat_buffer);
        _mat_buffer = nullptr;
        _mat_buffer_size = 0;
    }

    if (_av_packet)
    {
        av_packet_free(&_av_packet);
        _av_packet = nullptr;
    }

    if (_frame)
    {
        av_frame_free(&_frame);
        _frame = nullptr;
    }
    if (_sw_frame)
    {
        av_frame_free(&_sw_frame);
        _sw_frame = nullptr;
    }
    if (_frame_bgr)
    {
        av_frame_free(&_frame_bgr);
        _frame_bgr = nullptr;
    }

    if (_codec_context)
    {
        avcodec_free_context(&_codec_context);
        _codec_context = nullptr;
    }

    if (_sws_context)
    {
        sws_freeContext(_sws_context);
        _sws_context = nullptr;
    }

    if (_device_ctx)
    {
        av_buffer_unref(&_device_ctx);
        _device_ctx = nullptr;
    }

    if (_aviobuffer)
    {
        av_free(_aviobuffer);
        _aviobuffer = nullptr;
    }

    if (_avio)
    {
        avio_context_free(&_avio);
        _avio = nullptr;
    }

    if (_avformat)
    {
        avformat_close_input(&_avformat);
        avformat_free_context(_avformat);
        _avformat = nullptr;
    }

    while (!_framenode_que.empty())
    {
        framenode node = _framenode_que.front();
        _framenode_que.pop();

        free(node.data);
    }
}

void FFmpeger::DecodeMain()
{
    std::string err = "success";
    _decoding = Init(_codecId, err);
    while (_decoding)
    {
        do
        {
            int ret = av_read_frame(_avformat, _av_packet);
            if (ret < 0 || _av_packet->stream_index != _video_index)
            {
                break;
            }

            ret = avcodec_send_packet(_codec_context, _av_packet);
            if (ret >= 0)
            {
                ret = avcodec_receive_frame(_codec_context, _frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                else if (ret < 0)
                {
                    break;
                }

                if (_mat_buffer_size <= 0)
                {
                    _mat_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, _codec_context->width, _codec_context->height, 1);
                    _mat_buffer = (unsigned char*)av_malloc(_mat_buffer_size);
                    av_image_fill_arrays(_frame_bgr->data, _frame_bgr->linesize, _mat_buffer, AV_PIX_FMT_BGR24, _codec_context->width, _codec_context->height, 1);
                }

                AVFrame* tmp_frame = nullptr;
                if (_frame->format == _pixel_format)
                {
                    if ((ret = av_hwframe_transfer_data(_sw_frame, _frame, 0)) < 0)
                    {
                        fprintf(stderr, "error transferring the data to system memory\n");
                        break;
                    }
                    tmp_frame = _sw_frame;

                    if (!_sws_context)
                    {
                        _sws_context = sws_getContext(_codec_context->width, _codec_context->height, (AVPixelFormat)tmp_frame->format, _codec_context->width, _codec_context->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
                    }
                }
                else
                {
                    tmp_frame = _frame;

                    if (!_sws_context)
                    {
                        _sws_context = sws_getContext(_codec_context->width, _codec_context->height, _codec_context->pix_fmt, _codec_context->width, _codec_context->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
                    }
                }

                sws_scale(_sws_context, (const unsigned char* const*)tmp_frame->data, tmp_frame->linesize, 0, _codec_context->height, _frame_bgr->data, _frame_bgr->linesize);

                cv::Mat decodedFrame(cv::Size(_codec_context->width, _codec_context->height), CV_8UC3);
                memcpy(decodedFrame.data, _mat_buffer, _mat_buffer_size);

                cv::imshow(_source, decodedFrame);
                cv::waitKey(1);
            }
        } while (false);

        av_packet_unref(_av_packet);
    }
    Uninit();
}

int FFmpeger::DeviceInit(AVCodecContext *ctx, const enum AVHWDeviceType devType)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&_device_ctx, devType, NULL, NULL, 0)) < 0)
    {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(_device_ctx);

    return err;
}

enum AVPixelFormat FFmpeger::ChoosePixelFormat(const enum AVHWDeviceType devType)
{
    enum AVPixelFormat fmt = AV_PIX_FMT_NONE;

    switch (devType) {
    case AV_HWDEVICE_TYPE_VAAPI:
        fmt = AV_PIX_FMT_VAAPI;
        break;
    case AV_HWDEVICE_TYPE_DXVA2:
        fmt = AV_PIX_FMT_DXVA2_VLD;
        break;
    case AV_HWDEVICE_TYPE_D3D11VA:
        fmt = AV_PIX_FMT_D3D11;
        break;
    case AV_HWDEVICE_TYPE_VDPAU:
        fmt = AV_PIX_FMT_VDPAU;
        break;
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        break;
    default:
        fmt = AV_PIX_FMT_NONE;
        break;
    }

    return fmt;
}

enum AVPixelFormat FFmpeger::get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    const enum AVPixelFormat *ctx_p = (AVPixelFormat*)ctx->opaque;

    for (p = pix_fmts; *p != -1; p++) 
    {
        if (*p == *ctx_p)
        {
            return *p;
        }
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

FFmpeger::AVInitor::AVInitor()
{
    av_register_all();
    avformat_network_init();
}


