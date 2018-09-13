#include "RtmpPusher.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#include "PusherManager.h"

#pragma warning(disable:4244)
#pragma warning(disable:4996)

RtmpPusher::RtmpPusher(const std::string& rtmp)
    : _rtmp(rtmp)
    , _input_ctx(nullptr), _vedio_stream_index(-1), _live_ctx(nullptr)
    , _pushing(false), _push()
    , _url(), _locker(), _condition()
{
    _pushing = true;
    _push = std::thread(&RtmpPusher::Stream, this);
}

RtmpPusher::~RtmpPusher()
{
    if (_pushing)
    {
        _pushing = false;
        if (_push.joinable())
        {
            _push.join();
        }
    }
}

bool RtmpPusher::Push(const std::string& url, std::string& live)
{
    if (_pushing)
    {
        live = _rtmp + std::to_string(GetTickCount());
        _url = url;
        if (CreateRtmpContext(live))
        {
            std::lock_guard<std::mutex> lg(_locker);
            _condition.notify_one();
            return true;
        }
        return false;
    }
    live = "can not found stream service";
    return false;
}

void RtmpPusher::Stream()
{
    while (_pushing)
    {
        {
            std::unique_lock<std::mutex> ul(_locker);
            _condition.wait_for(ul, std::chrono::milliseconds(500), [this](){ return _input_ctx && _live_ctx; });
            if (!_input_ctx || !_live_ctx)
            {
                continue;
            }
        }

        int frame_index = 0;

        AVPacket pkt;
        int64_t start_time = av_gettime();
        while (_pushing) 
        {
            AVStream *in_stream, *out_stream;
            int ret = av_read_frame(_input_ctx, &pkt);
            if (ret < 0)
            {
                break;
            }

            if (pkt.pts == AV_NOPTS_VALUE)
            {
                //Write PTS
                AVRational time_base1 = _input_ctx->streams[_vedio_stream_index]->time_base;
                //Duration between 2 frames (us)
                int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(_input_ctx->streams[_vedio_stream_index]->r_frame_rate);
                //Parameters
                pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
                pkt.dts = pkt.pts;
                pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            }
            
            //Important:Delay
            if (pkt.stream_index == _vedio_stream_index)
            {
                AVRational time_base = _input_ctx->streams[_vedio_stream_index]->time_base;
                AVRational time_base_q = { 1, AV_TIME_BASE };
                int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
                int64_t now_time = av_gettime() - start_time;
                if (pts_time > now_time)
                {
                    av_usleep(pts_time - now_time);
                }
            }

            in_stream = _input_ctx->streams[pkt.stream_index];
            out_stream = _live_ctx->streams[pkt.stream_index];

            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
            pkt.pos = -1;
            
            if (pkt.stream_index == _vedio_stream_index)
            {
                frame_index++;
            }

            ret = av_interleaved_write_frame(_live_ctx, &pkt);
            if (ret < 0)
            {
                char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG(ERROR) << "push one live frame for file: " << _url << " failed: " << "(" << ret << ")" << errstr;
            }

            av_packet_unref(&pkt);
        }

        DestroyRtmpContext();

        PusherManager::Free(this);
    }
}

bool RtmpPusher::CreateRtmpContext(const std::string& live)
{
    int res = avformat_open_input(&_input_ctx, _url.c_str(), nullptr, nullptr);
    if (res < 0) 
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "open file: " << _url << " failed: " << "(" << res << ")" << errstr;
        return false;
    }

    res = avformat_find_stream_info(_input_ctx, 0);
    if (res < 0) 
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "find video stream for file: " << _url << " failed: " << "(" << res << ")" << errstr;
        return false;
    }

    for (int si = 0; si < (int)_input_ctx->nb_streams; si++)
    {
        if (_input_ctx->streams[si]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _vedio_stream_index = si;
            break;
        }
    }

#ifdef _DEVELOP_DEBUG
    av_dump_format(_input_ctx, 0, _url.c_str(), 0);
#endif

    res = avformat_alloc_output_context2(&_live_ctx, NULL, "flv", live.c_str()); //RTMP
    if (res < 0)
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "find video stream for file: " << _url << " failed: " << "(" << res << ")" << errstr;
        return false;
    }

    AVOutputFormat *ofmt = _live_ctx->oformat;
    for (int streamIdx = 0; streamIdx < (int)_input_ctx->nb_streams; streamIdx++)
    {
        AVStream *in_stream = _input_ctx->streams[streamIdx];
        AVStream *out_stream = avformat_new_stream(_live_ctx, in_stream->codec->codec);
        if (!out_stream)
        {
            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            LOG(ERROR) << "allocate live stream for file: " << _url << " failed: " << "(" << res << ")" << errstr;
            return false;
        }
        //¸´ÖÆAVCodecContextµÄÉèÖÃ£¨Copy the settings of AVCodecContext£©
        res = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (res < 0)
        {
            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            LOG(ERROR) << "configure live stream codec parameter for file: " << _url << " failed: " << "(" << res << ")" << errstr;
            return false;
        }
        out_stream->codec->codec_tag = 0;
        if (_live_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

#ifdef _DEVELOP_DEBUG
    av_dump_format(_live_ctx, 0, _rtmp.c_str(), 1);
#endif

    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        res = avio_open(&_live_ctx->pb, live.c_str(), AVIO_FLAG_WRITE);
        if (res < 0)
        {
            char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
            LOG(ERROR) << "open live for file: " << _url << " failed: " << "(" << res << ")" << errstr;
            return false;
        }
    }

    res = avformat_write_header(_live_ctx, NULL);
    if (res < 0)
    {
        char errstr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, res);
        LOG(ERROR) << "push live stream header for file: " << _url << " failed: " << "(" << res << ")" << errstr;
        return false;
    }
    return true;
}

void RtmpPusher::DestroyRtmpContext()
{
    av_write_trailer(_live_ctx);

    avformat_close_input(&_input_ctx);
    _input_ctx = nullptr;

    /* close output */
    if (_live_ctx && !(_live_ctx->oformat->flags & AVFMT_NOFILE))
        avio_close(_live_ctx->pb);

    avformat_free_context(_live_ctx);
    _live_ctx = nullptr;
}


