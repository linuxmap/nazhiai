
#ifndef _AVPACKETMUXTASK_HEADER_H_
#define _AVPACKETMUXTASK_HEADER_H_

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

class AvPacketMuxTaskPool;
class AvPacketMuxTask
{
public:
    AvPacketMuxTask(int hostId, const std::string& url, AVFormatContext** from, int width, int height, int frameRate, AvPacketMuxTaskPool& pool);
    ~AvPacketMuxTask();

    void TrigeNewMux(const std::string& absolute, const std::string& relative);
    void WaitToStop();

    void AddPacket(AVPacket* packet, long long frame_index);

    int64_t Duration();

    inline void SetBegin(long long beg) { _begin = beg; }
    inline void SetEnd(long long ed) { _end = ed; }

private:
    void Mux();

    bool CreateContext();
    void DestroyContext();

private:
    int _hostId;
    std::string _url;

    std::string _absolute;
    std::string _relative;

    int     _frq;
    int64_t _duration;

    bool    _first_packet;
    int64_t _prev_pts;
    int64_t _prev_dts;

    AVFormatContext* _avformatCxt;
    bool _write_header_success;

    AVRational _timebase;

    AVFormatContext** _avformatCxtFrom;

    bool _muxing;
    std::thread _mux;

    std::queue<AVPacket*> _packets;
    std::mutex _locker;
    std::condition_variable _condition;
    
    long long _begin;
    long long _end;

    AvPacketMuxTaskPool& _pool;
};

#endif

