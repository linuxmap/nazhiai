
#include "Dxva2Decoder.h"
#include <iostream>

// AVStream::codec
#pragma warning(disable:4996)

static char av_error_message_buffer[512] = { 0 };

#define GET_AV_ERR_MESSAGE(err) \
    memset(av_error_message_buffer, 0, 512);\
    av_strerror(err, av_error_message_buffer, 512)


Dxva2Decoder::Dxva2Decoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, const AvOptions& avOptions, HWND hWnd)
    : BaseDecoder(url, decoderParam, decodeParam, id)
    , _avOptions(avOptions)
    , _daemonThread(), _watchDog(false), _daemonLocker(), _daemonCondition()
    , _readPacketThread(), _readingPacket(false), _hasReadStartFrame(false), _packetBuffer(100)
    , _hWnd(hWnd), _decodePacketQueLocker(), _decodePacketQueCondition(), _decodePacketQue()
    , _formatContext(avformat_alloc_context()), _codecContext(nullptr), _swsContextForCPU(nullptr), _swsContextForGPU(nullptr)
    , _streamIndex(-1), _avStream(nullptr), _decodeInfo(nullptr)
    , _srcAvFrame(nullptr), _dstAvFrame(nullptr), _bgrAvFrame(nullptr)
    , _avBuffer(nullptr), _bufferSize(0)
{
}

Dxva2Decoder::~Dxva2Decoder()
{
}

void Dxva2Decoder::Create() throw(BaseException)
{
    StartReadPacket();
    StartDaemon();
    BaseDecoder::Create();
}

void Dxva2Decoder::Destroy() throw(BaseException)
{
    StopDameon();
    StopReadPacket();

    // clear buffer
    _packetBuffer.Clear();

    BaseDecoder::Destroy();
}

static int dxva2_retrieve_data_copy(AVCodecContext *s, AVFrame *frame, AVFrame *dstframe)
{
    LPDIRECT3DSURFACE9 surface = (LPDIRECT3DSURFACE9)frame->data[3];
    InputStream        *ist = (InputStream *)s->opaque;
    DXVA2Context       *ctx = (DXVA2Context *)ist->hwaccel_ctx;

    D3DSURFACE_DESC    surfaceDesc;
    D3DLOCKED_RECT     LockedRect;
    HRESULT            hr;
    int                ret;

    IDirect3DSurface9_GetDesc(surface, &surfaceDesc);

    ctx->tmp_frame->width = frame->width;
    ctx->tmp_frame->height = frame->height;
    ctx->tmp_frame->format = AV_PIX_FMT_NV12;

    ret = av_frame_get_buffer(ctx->tmp_frame, 32);
    if (ret < 0)
        return ret;

    hr = IDirect3DSurface9_LockRect(surface, &LockedRect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) {
        av_log(NULL, AV_LOG_ERROR, "Unable to lock DXVA2 surface\n");
        return AVERROR_UNKNOWN;
    }

    av_image_copy_plane(ctx->tmp_frame->data[0], ctx->tmp_frame->linesize[0],
        (uint8_t*)LockedRect.pBits,
        LockedRect.Pitch, frame->width, frame->height);

    av_image_copy_plane(ctx->tmp_frame->data[1], ctx->tmp_frame->linesize[1],
        (uint8_t*)LockedRect.pBits + LockedRect.Pitch * surfaceDesc.Height,
        LockedRect.Pitch, frame->width, frame->height / 2);

    IDirect3DSurface9_UnlockRect(surface);
    ret = av_frame_copy_props(ctx->tmp_frame, dstframe);
    if (ret < 0)
        goto fail;

    av_frame_unref(dstframe);
    av_frame_move_ref(dstframe, ctx->tmp_frame);

    return 0;
fail:
    av_frame_unref(ctx->tmp_frame);
    return ret;
}


bool Dxva2Decoder::ReadFrame(cv::Mat& frame)
{
    AVPacket* packet(nullptr);
    {
        WAIT_MILLISEC_TILL_COND(_decodePacketQueCondition, _decodePacketQueLocker, 50, [this](){ return _decodePacketQue.size() > 0; });
        if (_decodePacketQue.size() > 0)
        {
            packet = _decodePacketQue.front();
            _decodePacketQue.pop();
        }
    }

    bool readFrameOk = false;
    if (!packet)
    {
        return readFrameOk;
    }

    int decodedFrameCount = 0;
    int res = avcodec_decode_video2(_codecContext, _srcAvFrame, &decodedFrameCount, packet);
    if (res >= 0)
    {
        if (decodedFrameCount > 0)
        {
            // CPU decoding
            if (_swsContextForCPU)
            {
                sws_scale(
                    _swsContextForCPU,
                    (const uint8_t* const*)_srcAvFrame->data,
                    _srcAvFrame->linesize,
                    0,
                    _codecContext->height,
                    _bgrAvFrame->data,
                    _bgrAvFrame->linesize);

                cv::Mat decodedFrame(cv::Size(_codecContext->width, _codecContext->height), CV_8UC3);
                memcpy(decodedFrame.data, _avBuffer, _bufferSize);
                frame = decodedFrame;
                
                readFrameOk = true;
            }
            else
            {
                // copy data from GPU to CPU
                if (0 == dxva2_retrieve_data_copy(_codecContext, _srcAvFrame, _dstAvFrame))
                {
                    GetGPUSWSContext(_swsContextForGPU, *_dstAvFrame);

                    sws_scale(
                        _swsContextForGPU,
                        (const uint8_t* const*)_dstAvFrame->data,
                        _dstAvFrame->linesize,
                        0,
                        _dstAvFrame->height,
                        _bgrAvFrame->data,
                        _bgrAvFrame->linesize);

                    cv::Mat decodedFrame(cv::Size(_dstAvFrame->width, _dstAvFrame->height), CV_8UC3);
                    memcpy(decodedFrame.data, _avBuffer, _bufferSize);
                    frame = decodedFrame;

                    readFrameOk = true;
                }
                else
                {
                    printf("failed to copy data from GPU to CPU when decoding image frame\n");
                }
            }
        }
    }
    else
    {
        printf("decode image frame from packet failed");
    }

    _packetBuffer.Free(packet);

    return readFrameOk;
}

bool Dxva2Decoder::ReadFrame(cv::cuda::GpuMat& frame)
{
    cv::Mat mat;
    if (ReadFrame(mat))
    {
        frame.upload(mat);
        return true;
    }
    return false;
}

void Dxva2Decoder::StartReadPacket()
{
    AVDictionary* pOptions = NULL;
    for (auto opt : _avOptions)
    {
        if (av_dict_set(&pOptions, opt.first.c_str(), opt.second.c_str(), 0) < 0)
        {
            printf("set av option failed(%s:%s)\n", opt.first.c_str(), opt.second.c_str());
        }
    }

    int avResult = avformat_open_input(&_formatContext, _url.c_str(), NULL, &pOptions);
    if (avResult < 0)
    {
        GET_AV_ERR_MESSAGE(avResult);
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: " + av_error_message_buffer);
    }
    _formatContext->interrupt_callback = { ReadInterruptCb, this };

    if (!_formatContext || (avResult = avformat_find_stream_info(_formatContext, NULL)) < 0)
    {
        GET_AV_ERR_MESSAGE(avResult);
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: " + av_error_message_buffer);
    }

    // find the first video stream
    for (unsigned int i = 0; i < _formatContext->nb_streams; i++)
    {
        if (_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _streamIndex = i;
            break;
        }
    }

    // if not found, then return failed
    if (_streamIndex == -1)
    {
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: video stream not found");
    }

    _avStream = _formatContext->streams[_streamIndex];
    _codecContext = _avStream->codec;
    if (!_codecContext)
    {
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: codec context not found");
    }

    _decodeInfo = avcodec_find_decoder(_codecContext->codec_id);
    if (!_decodeInfo)
    {
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: can not find decoder of codec id(" + std::to_string(_codecContext->codec_id) + ")");
    }

    if ((avResult = avcodec_open2(_codecContext, _decodeInfo, NULL)) < 0)
    {
        GET_AV_ERR_MESSAGE(avResult);
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: " + av_error_message_buffer);
    }

    if (_codecContext->width == 0 || _codecContext->height == 0)
    {
        throw BaseException(0, "DXVA2(" + _url + ") can not be opened: empty codec context");
    }

    // set video information
    _decodeParam.fps = (float)_avStream->avg_frame_rate.num * _avStream->avg_frame_rate.den;

    // allocate memory for decoder
    _srcAvFrame = av_frame_alloc(), _dstAvFrame = av_frame_alloc(), _bgrAvFrame = av_frame_alloc();

    // bind memory for frame
    _bufferSize = av_image_get_buffer_size(AV_PIX_FMT_BGR24, _codecContext->width, _codecContext->height, 1);
    _avBuffer = (unsigned char*)av_malloc(_bufferSize);
    av_image_fill_arrays(_bgrAvFrame->data, _bgrAvFrame->linesize, _avBuffer, AV_PIX_FMT_BGR24, _codecContext->width, _codecContext->height, 1);

    _codecContext->hw_device_ctx = nullptr;
    // multi-threading is apparently not compatible with hardware decoding
    _codecContext->thread_count = 1;

    InputStream *ist = new InputStream();
    ist->hwaccel_id = HWACCEL_AUTO;
    ist->active_hwaccel_id = HWACCEL_AUTO;
    ist->hwaccel_device = "dxva2";
    ist->dec = _decodeInfo;
    ist->dec_ctx = _codecContext;

    _codecContext->opaque = ist;

    bool initializeSuccess = false;
    if (_hWnd != NULL)
    {
        switch (_decodeInfo->id)
        {
        case AV_CODEC_ID_MPEG2VIDEO:
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_VC1:
        case AV_CODEC_ID_WMV3:
        case AV_CODEC_ID_HEVC:
        case AV_CODEC_ID_VP9:
        {
            // if initialize success, the decode thread will use GPU
            if (dxva2_init(_codecContext, _hWnd) == 0)
            {
                _codecContext->get_buffer2 = ist->hwaccel_get_buffer;
                _codecContext->get_format = Dxva2PixFormatCallback;
                _codecContext->thread_safe_callbacks = 1;

                initializeSuccess = true;
            }
        }
        }
    }

    if (!initializeSuccess)
    {
        // CPU context initialized, the decode thread will use CPU
        _swsContextForCPU = sws_getContext(
            _codecContext->width,
            _codecContext->height,
            _codecContext->pix_fmt,
            _codecContext->width,
            _codecContext->height,
            AV_PIX_FMT_BGR24,
            SWS_BICUBIC, // reference to http://www.cnblogs.com/mmix2009/p/3585524.html
            NULL,
            NULL,
            NULL);

        if (!_swsContextForCPU)
        {
            Destroy();
            throw BaseException(0, "DXVA2(" + _url + ") can not be opened: hardware and sws_context initialize failed");
        }
    }

    _readingPacket = true;
    _readPacketThread = std::thread(&Dxva2Decoder::ReadPacket, this);
}

void Dxva2Decoder::ReadPacket()
{
    while (_readingPacket)
    {
        // check packet validity
        AVPacket* packet = _packetBuffer.Alloc();
        if (!packet)
        {
            continue;
        }

        int errTimes = 0;
        int res = av_read_frame(_formatContext, packet);
        while (res < 0 && ++errTimes <= 100) {
            res = av_read_frame(_formatContext, packet);
        }

        // restart
        if (errTimes > 100) {
            _readingPacket = false;
            
            // local file synchronous mode
            if (res != AVERROR_EOF)
            {
                WAKEUP_ONE(_daemonCondition);
            }
        }

        // has read the packet
        if (res == 0)
        {
            if (packet->stream_index == _streamIndex)
            {
                if (_hasReadStartFrame)
                {
                    AUTOLOCK(_decodePacketQueLocker);
                    _decodePacketQue.push(packet);
                    WAKEUP_ONE(_decodePacketQueCondition);
                }
                else
                {
                    // check if is the key packet
                    if (packet->flags & AV_PKT_FLAG_KEY)
                    {
                        _hasReadStartFrame = true;
                        AUTOLOCK(_decodePacketQueLocker);
                        _decodePacketQue.push(packet);
                        WAKEUP_ONE(_decodePacketQueCondition);
                    }
                    else
                    {
                        // free packet without key flag
                        _packetBuffer.Free(packet);
                    }
                }
            }
            else
            {
                // decode audio frame
                _packetBuffer.Free(packet);
            }
        }
        else
        {
            _packetBuffer.Free(packet);
        }
    }
}


void Dxva2Decoder::StopReadPacket()
{
    if (_readingPacket)
    {
        _readingPacket = false;
        WAIT_TO_EXIT(_readPacketThread);
    }
    {
        // clear packet that ware not decoded
        AUTOLOCK(_decodePacketQueLocker);
        while (_decodePacketQue.size() > 0)
        {
            _packetBuffer.Free(_decodePacketQue.front());
            _decodePacketQue.pop();
        }
    }

    if (_formatContext)
    {
        avformat_close_input(&_formatContext);
        avformat_free_context(_formatContext);
        _formatContext = nullptr;
    }

    if (_swsContextForCPU)
    {
        sws_freeContext(_swsContextForCPU);
        _swsContextForCPU = nullptr;
    }
    if (_swsContextForGPU)
    {
        sws_freeContext(_swsContextForGPU);
        _swsContextForGPU = nullptr;
    }

    if (_srcAvFrame)
    {
        av_frame_free(&_srcAvFrame);
        _srcAvFrame = nullptr;
    }
    if (_dstAvFrame)
    {
        av_frame_free(&_dstAvFrame);
        _dstAvFrame = nullptr;
    }
    if (_bgrAvFrame)
    {
        av_frame_free(&_bgrAvFrame);
        _bgrAvFrame = nullptr;
    }

    if (_avBuffer)
    {
        av_free(_avBuffer);
        _avBuffer = nullptr;
        _bufferSize = 0;
    }
}

void Dxva2Decoder::StartDaemon()
{
    if (!_watchDog)
    {
        _watchDog = true;
        _daemonThread = std::thread(&Dxva2Decoder::Daemon, this);
    }
}

void Dxva2Decoder::Daemon()
{
    while (_watchDog)
    {
        WAIT(_daemonCondition, _daemonLocker);
        if (_watchDog)
        {
            StopReadPacket();
            StartReadPacket();
        }
    }
}

void Dxva2Decoder::StopDameon()
{
    if (_watchDog)
    {
        _watchDog = false;
        WAKEUP_ONE(_daemonCondition);
        WAIT_TO_EXIT(_daemonThread);
    }
}


