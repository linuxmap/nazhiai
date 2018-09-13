
#ifndef _RTSPSTREAMMUXTASK_HEADER_H_
#define _RTSPSTREAMMUXTASK_HEADER_H_

#include "AvPacketMuxTaskPool.h"
#include "MediaSystem.h"

class RtspStreamMuxTask
{
public:
    RtspStreamMuxTask(const std::string& rtsp, const std::string& fmt, int duration);
    virtual ~RtspStreamMuxTask();

    bool StartMux(std::string& result);
    void StopMux();

private:
    void Read();

    bool OpenRtsp(std::string& result);
    void CloseRtsp();

    AvPacketMuxTask* StartAvPacketMuxTask();
    void StopAvPacketMuxTask(AvPacketMuxTask** task);

    void GuessIPFromRtsp();

private:
    std::string _rtsp;
    std::string _extention;

    int _duration;

    AVFormatContext* _avformatCxt;

    bool _reading;
    std::thread _read;

    int    _width, _height;
    double _frame_rate;
    long long _frame_index;

    MediaSystem::MFS _mfs;
    int _host_id;
    std::string _rtsp_ip;

    AvPacketMuxTaskPool _taskpool;
};

#endif

