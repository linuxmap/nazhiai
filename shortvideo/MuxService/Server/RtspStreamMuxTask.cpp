
#include "RtspStreamMuxTask.h"

#include "AvPacketPool.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "TaskManager.h"

#include "Maria.h"

#include <chrono>
#include <map>
#include <sstream>

//////////////////////////////////////////////////////////////

static long long Now()
{
    typedef std::chrono::milliseconds time_precision;
    typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
    now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
    return tp.time_since_epoch().count();
}

RtspStreamMuxTask::RtspStreamMuxTask(const std::string& rtsp, const std::string& fmt, int duration)
    : _rtsp(rtsp), _extention(fmt), _duration(duration)
    , _avformatCxt(nullptr)
    , _reading(false), _read()
    , _mfs(), _host_id(-1), _rtsp_ip()
    , _taskpool()
{
    _width = _height = 0;
    _frame_rate = 25.0f;
    _frame_index = 0l;
}

RtspStreamMuxTask::~RtspStreamMuxTask()
{
}

bool RtspStreamMuxTask::StartMux(std::string& result)
{
    if (!_rtsp.empty() && !_reading)
    {
        Maria maria(TaskManager::db.url);
        // query host information
        if (!maria.OneHost(TaskManager::fs.host, TaskManager::fs.root, _host_id, std::string()))
        {
            // create one record
            if (!maria.OneHost(TaskManager::fs.host, TaskManager::fs.root, std::string()))
            {
                return false;
            }

            // query host information
            if (!maria.OneHost(TaskManager::fs.host, TaskManager::fs.root, _host_id, std::string()))
            {
                return false;
            }
        }

        GuessIPFromRtsp();

        // find the path and open the rtsp
        if (MediaSystem::NavigateToNearestPath(TaskManager::fs.root, _rtsp_ip, TaskManager::fs.max_files, _mfs) && OpenRtsp(result))
        {
            _reading = true;
            _read = std::thread(&RtspStreamMuxTask::Read, this);
            return true;
        } 
        CloseRtsp();
    }
    return false;
}


void RtspStreamMuxTask::StopMux()
{
    if (_reading)
    {
        _reading = false;
        if (_read.joinable())
        {
            _read.join();
        }
        CloseRtsp();
    }
}

bool RtspStreamMuxTask::OpenRtsp(std::string& result)
{
    if (!_avformatCxt)
    {
        _frame_index = 0;
        std::map<std::string, std::string> avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
        avOptions.insert(std::make_pair("analyzeduration", "20000000"));
        avOptions.insert(std::make_pair("probesize", "20000000"));
        avOptions.insert(std::make_pair("rtsp_transport", "tcp"));
        AVDictionary* pOptions = NULL;
        for (auto opt : avOptions)
        {
            if (av_dict_set(&pOptions, opt.first.c_str(), opt.second.c_str(), 0) < 0)
            {
                LOG(WARNING) << "set av option failed(" << opt.first << ":" << opt.second << ")";
            }
        }

        int res = avformat_open_input(&_avformatCxt, _rtsp.c_str(), 0, &pOptions);
        if (res < 0)
        {
            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            std::stringstream ss;
            ss << "open rtsp: " << _rtsp << " failed: " << "(" << res << ")" << errstr;
            result = ss.str();
            LOG(ERROR) << result;
            return false;
        }

        if (avformat_find_stream_info(_avformatCxt, 0) < 0)
        {
            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            std::stringstream ss;
            ss << "open rtsp: " << _rtsp << " failed, can not find stream info: (" << res << ")" << errstr;
            result = ss.str();
            LOG(ERROR) << result;
            return false;
        }

#ifdef _DEVELOP_DEBUG
        av_dump_format(_avformatCxt, 0, _rtsp.c_str(), 0);
#endif

        for (unsigned int i = 0; i < _avformatCxt->nb_streams; i++)
        {
            AVStream *stream = _avformatCxt->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                _width = stream->codecpar->width;
                _height = stream->codecpar->height;

                if (stream->avg_frame_rate.den != 0 && stream->avg_frame_rate.num != 0)
                {
                    _frame_rate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
                }
            }
        }

        std::stringstream ss;
        ss << "open rtsp: " << _rtsp << " success: width(" << _width << "), height(" << _height << "), frame rate: " << _frame_rate;
        result = ss.str();
        LOG(INFO) << result;

        Maria maria(TaskManager::db.url);
        maria.ActivateSouce(_host_id, _rtsp, Now());

        return true;
    }

    return false;
}

void RtspStreamMuxTask::CloseRtsp()
{
    if (_avformatCxt)
    {
        avformat_close_input(&_avformatCxt);
        _avformatCxt = nullptr;

        Maria maria(TaskManager::db.url);
        maria.DeactivateSouce(_host_id, _rtsp);
    }
}

AvPacketMuxTask* RtspStreamMuxTask::StartAvPacketMuxTask()
{
    if (MediaSystem::NextMediaPath(TaskManager::fs.max_files, _mfs))
    {
        AvPacketMuxTask* task = _taskpool.Alloc(_host_id, _rtsp, &_avformatCxt, _width, _height, (int)_frame_rate);
        if (task)
        {
            long long beg = Now();
            task->SetBegin(beg);
            std::string absolute = _mfs.spath + std::to_string(beg) + _extention;
            task->TrigeNewMux(absolute, absolute.substr(TaskManager::fs.root.size()));
        }
        return task;
    }
    return nullptr;
}

void RtspStreamMuxTask::StopAvPacketMuxTask(AvPacketMuxTask** task)
{
    if (*task)
    {
        (*task)->SetEnd(Now());
        (*task)->AddPacket(nullptr, 0);
        (*task) = nullptr;
    }
}

void RtspStreamMuxTask::GuessIPFromRtsp()
{
    // rtsp://admin:admin123@192.168.2.64/h264
    size_t beg = 0, ed = 0;
    int count = 0;
    for (size_t idx = 3; idx < _rtsp.size() && count < 3; ++idx)
    {
        if (_rtsp[idx] == '.')
        {
            if (beg == 0)
            {
                beg = idx;
                count++;
            } 
            else
            {
                ed = idx;
                count++;
            }
        }
    }

    // not found IP information
    if (ed <= beg)
    {
        for (size_t i = 0; i < _rtsp.size(); ++i)
        {
            char ch = _rtsp[i];
            if ((ch >= '0' && ch <= '9')
                || (ch >= 'a' && ch <= 'z')
                || (ch >= 'A' && ch <= 'Z'))
            {
                _rtsp_ip.push_back(ch);
            }
            else
            {
                _rtsp_ip.push_back('_');
            }
        }
    }
    else
    {
        while (--beg >= 0)
        {
            char ch = _rtsp[beg];
            if (ch < '0' || ch > '9')
            {
                break;
            }
        }
        beg++;

        while(++ed < _rtsp.size())
        {
            char ch = _rtsp[ed];
            if (ch < '0' || ch > '9')
            {
                break;
            }
        }

        _rtsp_ip = _rtsp.substr(beg, ed - beg);
    }

    if (_rtsp_ip.size() > 64)
    {
        _rtsp_ip = _rtsp_ip.substr(0, 64);
    }
}

void RtspStreamMuxTask::Read()
{
    AvPacketMuxTask* task = StartAvPacketMuxTask();
    AVStream** streams = _avformatCxt->streams;
    while (_reading)
    {
        AVPacket* packet = AvPacketPool::Alloc();
        if (!packet)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        int res = av_read_frame(_avformatCxt, packet);
        if (res < 0)
        {
            AvPacketPool::Free(packet);

            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            
            if (_reading)
            {
                LOG(WARNING) << "read rtsp: " << _rtsp << " failed: (" << res << ")" << errstr;
                LOG(WARNING) << _rtsp << " is going to restart...";

                StopAvPacketMuxTask(&task);
                CloseRtsp();

                std::string err;
                if (OpenRtsp(err))
                {
                    task = StartAvPacketMuxTask();
                    if (task)
                    {
                        streams = _avformatCxt->streams;
                        LOG(WARNING) << _rtsp << " restart success";
                    }
                    continue;
                } 
                else
                {
                    LOG(WARNING) << _rtsp << " restart failed: " << err << " and this will retry 1 second later";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }
            else
            {
                LOG(ERROR) << "read rtsp: " << _rtsp << " failed: (" << res << ")" << errstr;
                break;
            }
        }
        else
        {
            if (!task)
            {
                AvPacketPool::Free(packet);
                task = StartAvPacketMuxTask();
                continue;
            }
        }

        if (streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (task->Duration() >= _duration)
            {
                if ((packet->flags & AV_PKT_FLAG_KEY) != AV_PKT_FLAG_KEY)
                {
                    task->AddPacket(packet, _frame_index);
                }
                else
                {
                    StopAvPacketMuxTask(&task);
                    task = StartAvPacketMuxTask();
                    if (task)
                    {
                        task->AddPacket(packet, _frame_index);
                    }
                    else
                    {
                        AvPacketPool::Free(packet);
                    }
                }
            }
            else
            {
                task->AddPacket(packet, _frame_index);
            }
            ++_frame_index;
        }
        else
        {
            AvPacketPool::Free(packet);
        }
    }

    if (task)
    {
        task->WaitToStop();
        delete task;
        task = nullptr;
    }
}




