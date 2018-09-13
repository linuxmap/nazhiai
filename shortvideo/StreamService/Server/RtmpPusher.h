
#ifndef _RTMPPUSHER_HEADER_H_
#define _RTMPPUSHER_HEADER_H_

#pragma warning(disable:4819)

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
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

#pragma warning(default:4819)

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

class RtmpPusher
{
public:
    RtmpPusher(const std::string& rtmp);
    ~RtmpPusher();

    bool Push(const std::string& url, std::string& live);

private:
    void Stream();

    bool CreateRtmpContext(const std::string& live);
    void DestroyRtmpContext();

private:
    const std::string& _rtmp;

    AVFormatContext* _input_ctx;
    int _vedio_stream_index;
    AVFormatContext* _live_ctx;

    bool _pushing;
    std::thread _push;

    std::string _url;
    std::mutex _locker;
    std::condition_variable _condition;

private:
    RtmpPusher(const RtmpPusher&);
    RtmpPusher& operator=(const RtmpPusher&);
};


#endif

