#ifndef _DXVA2DECODER_HEADER_H_
#define _DXVA2DECODER_HEADER_H_

#include "BaseDecoder.h"

#include "dxva2/ffmpeg_dxva2.h"

class Dxva2Decoder : public BaseDecoder
{
public:
    Dxva2Decoder(const std::string& url, const DecoderParam&, const DecodeParam&, const std::string&, const AvOptions&, HWND);
    ~Dxva2Decoder();

    const char* Name() const
    {
        return "Dxva2Decoder";
    }

protected:
    bool Init();
    void Uninit();
    bool ReadFrame(DecodedFrame& frame) override;

private:
    bool StartReadPacket();
    void ReadPacket();
    void StopReadPacket();

    // avformat_open_input/av_read_frame timeout callback
    // return: 0(continue original call), other(interrupt original call)
    inline static int ReadInterruptCb(void* context)
    {
        Dxva2Decoder* decoder = (Dxva2Decoder*)context;
        /*if (decoder && decoder->Stop()) {
            decoder->Start();
            return 1;
        }*/
        return 0;
    }

    inline static AVPixelFormat Dxva2PixFormatCallback(AVCodecContext* avCodecCtx, const AVPixelFormat*)
    {
        InputStream* ist = (InputStream*)avCodecCtx->opaque;
        ist->active_hwaccel_id = HWACCEL_DXVA2;
        ist->hwaccel_pix_fmt = AV_PIX_FMT_DXVA2_VLD;
        return ist->hwaccel_pix_fmt;
    }

    inline void GetGPUSWSContext(SwsContext*& swsContext, AVFrame& avFrame)
    {
        if (!swsContext)
        {
            swsContext = sws_getContext(
                avFrame.width,
                avFrame.height,
                (AVPixelFormat)avFrame.format,
                avFrame.width,
                avFrame.height,
                AV_PIX_FMT_BGR24,
                SWS_BICUBIC, // reference to http://www.cnblogs.com/mmix2009/p/3585524.html
                NULL,
                NULL,
                NULL);
        }
    }

private:
    AvOptions _avOptions;

    // ---- read packet ----
    std::thread _readPacketThread;
    bool _readingPacket;
    bool _hasReadStartFrame;

    AvPacketBuffer _packetBuffer;

    // ---- decode video frame ----
    HWND _hWnd;
    std::string _deviceIndex;

    std::mutex _decodePacketQueLocker;
    std::condition_variable _decodePacketQueCondition;
    PacketQue _decodePacketQue;

    AVFormatContext* _formatContext;
    AVCodecContext* _codecContext;
    SwsContext* _swsContextForCPU;
    SwsContext* _swsContextForGPU;

    // ---- decoded information ----
    int _streamIndex;
    AVStream* _avStream;
    AVCodec* _decodeInfo;

    AVFrame* _srcAvFrame;
    AVFrame* _dstAvFrame;
    AVFrame* _bgrAvFrame;

    // ---- decode buffer information ----
    unsigned char* _avBuffer;
    size_t _bufferSize;

private:
    Dxva2Decoder();
    Dxva2Decoder(const Dxva2Decoder&);
    Dxva2Decoder& operator=(const Dxva2Decoder&);
};

#endif
