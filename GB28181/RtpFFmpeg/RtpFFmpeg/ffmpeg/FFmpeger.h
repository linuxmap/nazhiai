
#ifndef _FFMPEGER_HEADER_H_
#define _FFMPEGER_HEADER_H_

#pragma warning(disable:4819)
#pragma warning(disable:4996)
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libavutil/avassert.h>
#include <libavutil/hwcontext.h>
#include <libavcodec/dxva2.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/buffer.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <opencv2\opencv.hpp>

#pragma warning(default:4819)
#pragma warning(default:4996)

class FFmpeger {
private:
    class AVInitor{
    public:
        AVInitor();
    };

public:
    static FFmpeger* CreateFFmpeger(char payloadType, const std::string& source);
    static void DestroyFFmpeger(FFmpeger*);

public:
    virtual ~FFmpeger();

    void FeedFrame(unsigned char* packet, size_t size);
    void GotFrame(unsigned char* frame, int* size);

protected:
    bool Init(AVCodecID codecid, std::string& err);
    void Uninit();

    void DecodeMain();

    int DeviceInit(AVCodecContext *ctx, const enum AVHWDeviceType devType);
    enum AVPixelFormat ChoosePixelFormat(const enum AVHWDeviceType devType);

    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);
    
protected:
    enum AVCodecID _codecId;
    std::string _source;

    // AVIO information
    AVFormatContext* _avformat;
    AVIOContext* _avio;
    unsigned char * _aviobuffer;
    int _video_index;

    // AVIO input split information
    unsigned char* _origin_data;
    unsigned char* _left_data;
    int _left_size;

    // CODEC information
    AVCodec* _codec;
    AVCodecContext* _codec_context;
    AVPacket* _av_packet;
    AVFrame* _frame;
    AVFrame* _sw_frame;
    AVFrame* _frame_bgr;
    SwsContext* _sws_context;
    
    // hardware information
    enum AVPixelFormat _pixel_format;
    AVBufferRef *_device_ctx;

    // decoded image buffer information
    unsigned char* _mat_buffer;
    int _mat_buffer_size;

    // decode thread information
    std::thread _decode;
    std::mutex _locker;
    std::condition_variable _condition;
    bool _decoding;

    // frame data from network
    struct framenode
    {
        unsigned char* data;
        size_t size;
    };
    typedef std::queue<framenode> framenode_que;
    framenode_que _framenode_que;

private:
    FFmpeger(enum AVCodecID codecId, const std::string& source);

private:
    static int AVIORead(void* opaque, unsigned char* buf, int size);

private:
    static AVInitor av_registeror;

private:
    FFmpeger();
    FFmpeger(const FFmpeger&);
    FFmpeger& operator=(const FFmpeger&);
};

#endif

