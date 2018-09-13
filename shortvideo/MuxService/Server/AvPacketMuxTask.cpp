#include "AvPacketMuxTask.h"

#include "AvPacketPool.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "Maria.h"
#include "AvPacketMuxTaskPool.h"
#include "TaskManager.h"
#include "FileSystem.h"

#pragma warning(disable:4244)
#pragma warning(disable:4996)

AvPacketMuxTask::AvPacketMuxTask(int hostId, const std::string& url, AVFormatContext** from, int width, int height, int frameRate, AvPacketMuxTaskPool& pool)
    : _hostId(hostId), _url(url)
    , _absolute(), _relative()
    , _frq(0), _duration(0)
    , _first_packet(true), _prev_pts(-1), _prev_dts(-1)
    , _avformatCxt(nullptr), _write_header_success(false)
    , _timebase()
    , _avformatCxtFrom(from)
    , _muxing(false), _mux()
    , _packets(), _locker(), _condition()
    , _begin(0), _end(0)
    , _pool(pool)
{
    _timebase.num = 1;
    _timebase.den = frameRate;
    _muxing = true;
    _mux = std::thread(&AvPacketMuxTask::Mux, this);
}

AvPacketMuxTask::~AvPacketMuxTask()
{
}

void AvPacketMuxTask::TrigeNewMux(const std::string& absolute, const std::string& relative)
{
    if (absolute.empty() || relative.empty())
    {
        LOG(ERROR) << "start new packet mux task failed, because the file target file path is empty";
    } 
    else
    {
        std::lock_guard<std::mutex> lg(_locker);
        _first_packet = true;

        _frq = 0;
        _duration = 0;

        _absolute = absolute;
        _relative = relative;
    }
}

void AvPacketMuxTask::WaitToStop()
{
    _muxing = false;
    if (_mux.joinable())
    {
        _mux.join();
    }
}

void AvPacketMuxTask::AddPacket(AVPacket* packet, long long frame_index)
{
    if (packet)
    {
        AVFormatContext* from = *_avformatCxtFrom;
        // if the first packet is not the key packet, discard it
        if (_first_packet && (packet->flags & AV_PKT_FLAG_KEY) != AV_PKT_FLAG_KEY)
        {
            AvPacketPool::Free(packet);
            return;
        }

        AVStream *stream_from = from->streams[packet->stream_index];

        if (packet->pts == AV_NOPTS_VALUE){
            //Write PTS
            AVRational time_base1 = from->streams[packet->stream_index]->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(from->streams[packet->stream_index]->r_frame_rate);
            //Parameters
            packet->pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            packet->dts = packet->pts;
            packet->duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
        }

        packet->pts = av_rescale_q_rnd(packet->pts, stream_from->time_base, _timebase, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet->dts = av_rescale_q_rnd(packet->dts, stream_from->time_base, _timebase, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

        if (_first_packet)
        {
            _first_packet = false;
            _prev_pts = packet->pts;
            _prev_dts = packet->dts;
        }
        if (packet->pts >= _prev_pts)
        {
            packet->pts -= _prev_pts;
        }
        if (packet->dts >= _prev_dts)
        {
            packet->dts -= _prev_dts;
        }

        packet->duration = av_rescale_q(packet->duration, stream_from->time_base, _timebase);
        packet->pos = -1;

        if (++_frq >= _timebase.den)
        {
            _frq = 0;
            _duration++;
        }
    }

    std::lock_guard<std::mutex> lg(_locker);
    _packets.push(packet);
    _condition.notify_one();
}

int64_t AvPacketMuxTask::Duration()
{
    return _duration;
}

void AvPacketMuxTask::Mux()
{
    while (_muxing || _packets.size() > 0)
    {
        AVPacket* packet = nullptr;
        {
            std::unique_lock<std::mutex> ul(_locker);
            _condition.wait_for(ul, std::chrono::milliseconds(500), [this](){ return _packets.size() > 0; });
            if (_packets.size() == 0)
            {
                continue;
            }

            // new muxing task incoming
            if (!_avformatCxt)
            {
                if (!CreateContext())
                {
                    DestroyContext();
                }
            }

            // fetch packet
            packet = _packets.front();
            _packets.pop();

            // muxing task starting failed, then ignore all packet
            if (!_avformatCxt)
            {
                if (packet)
                {
                    AvPacketPool::Free(packet);
                }
                continue;
            }

            // find the last nullptr packet
            if (!packet)
            {
                DestroyContext();

                // this task is free now, can be used at next time
                _pool.Free(this);

                continue;
            }
        }

        AVStream *stream_from = (*_avformatCxtFrom)->streams[packet->stream_index];

        if (stream_from->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            // write the compressed frame to the output format
            int res = av_interleaved_write_frame(_avformatCxt, packet);
            if (res != 0)
            {
                char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);

                LOG(ERROR) << "muxing error(" << res << ") while writing video frame for " << _absolute << ": " << errstr;
            }
        }
        else if (stream_from->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            // write the compressed frame to the output format
            int res = av_interleaved_write_frame(_avformatCxt, packet);
            if (res != 0)
            {
                char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);

                LOG(ERROR) << "muxing error(" << res << ") while writing audio frame for " << _absolute << ": " << errstr;
            }
        }

        AvPacketPool::Free(packet);
    }
}

bool AvPacketMuxTask::CreateContext()
{
    std::string extension = "avi";
    std::string::size_type pos = _absolute.find_last_of('.');
    if (pos != std::string::npos)
    {
        extension = _absolute.substr(pos + 1);
    }

    int res = avformat_alloc_output_context2(&_avformatCxt, nullptr, extension.c_str(), _absolute.c_str());

    if (res < 0)
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE + 1] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "allocate muxing context for " << _absolute << " failed: " << errstr;
        return false;
    }
    _write_header_success = false;

    AVOutputFormat* fmt = _avformatCxt->oformat;
    for (unsigned int i = 0; i < (*_avformatCxtFrom)->nb_streams; i++)
    {
        AVStream *stream_from = (*_avformatCxtFrom)->streams[i];
        if (stream_from->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVCodecContext* codec_ctx_from = stream_from->codec;
            AVStream *stream_to = avformat_new_stream(_avformatCxt, nullptr);
            if (!stream_to)
            {
                LOG(ERROR) << "allocate new stream for " << _absolute << " failed";
                return false;
            }
            stream_to->id = _avformatCxt->nb_streams - 1;

            res = avcodec_parameters_from_context(stream_to->codecpar, codec_ctx_from);
            if (res < 0)
            {
                char errstr[AV_ERROR_MAX_STRING_SIZE + 1] = { 0 };
                av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
                LOG(ERROR) << "parse muxing parameters for " << _absolute << " failed: " << errstr;
                return false;
            }

            AVCodecContext* codec_ctx = avcodec_alloc_context3(NULL);
            res = avcodec_parameters_to_context(codec_ctx, stream_to->codecpar);
            if (res < 0)
            {
                char errstr[AV_ERROR_MAX_STRING_SIZE + 1] = { 0 };
                av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
                LOG(ERROR) << "convert muxing parameters to codec context for " << _absolute << " failed: " << errstr;
                return false;
            }

            codec_ctx->extradata = codec_ctx_from->extradata;
            codec_ctx->extradata_size = codec_ctx_from->extradata_size;
            codec_ctx->bit_rate = codec_ctx_from->bit_rate;

            if (fmt->flags & AVFMT_GLOBALHEADER)
            {
                codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            
            stream_to->time_base = _timebase;

            codec_ctx->time_base = _timebase;
            codec_ctx->gop_size = 12; /* emit one intra frame every twelve frames at most */

            if (codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            {
                /* just for testing, we also add B frames */
                codec_ctx->max_b_frames = 2;
            }
            if (codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            {
                /* Needed to avoid using macroblocks in which some coeffs overflow.
                * This does not happen with normal video, it just happens here as
                * the motion of the chroma plane does not match the luma plane. */
                codec_ctx->mb_decision = 2;
            }
        }
    }

#ifdef _DEVELOP_DEBUG
    av_dump_format(_avformatCxt, 0, _absolute.c_str(), 1);
#endif

    res = avio_open(&_avformatCxt->pb, _absolute.c_str(), AVIO_FLAG_WRITE);
    if (res < 0)
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE + 1] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "open muxing file " << _absolute << " failed: " << errstr;
        return false;
    }

    AVDictionary* opt = NULL;
    av_dict_set_int(&opt, "video_track_timescale", _timebase.den, 0);
    res = avformat_write_header(_avformatCxt, &opt);
    if (res < 0)
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE + 1] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "write muxing file header for " << _absolute << " failed: " << errstr;
        return false;
    }
    _write_header_success = true;

    return true;
}

void AvPacketMuxTask::DestroyContext()
{
    if (_avformatCxt)
    {
        bool ending_success = _write_header_success;
        if (_write_header_success)
        {
            ending_success = av_write_trailer(_avformatCxt) == 0;
        }

        if (!(_avformatCxt->flags & AVFMT_NOFILE))
        {
            avio_close(_avformatCxt->pb);
        }

        if (ending_success)
        {
            Maria maria(TaskManager::db.url);
            ending_success = maria.OneItem(_hostId, _url, _begin, _end, _relative, std::string());
        }

        // if write header, tail or save database failed, then remove this file(because this video can not be played)
        if (!ending_success && FileSystem::Accessible(_absolute, _A_NORMAL))
        {
            FileSystem::RemoveFile(_absolute);
        }

        avformat_free_context(_avformatCxt);
        _avformatCxt = nullptr; 
    }
}

