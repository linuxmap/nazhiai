#include "StreamParsor.h"

#include "ffmpeg_dxva2.h"

#include <sstream>
#include <map>

#pragma warning(disable:4819)
#pragma warning(disable:4996)

StreamParsor::StreamParsor(const std::string& url)
    : _url(url)
{
}

StreamParsor::~StreamParsor()
{
}

bool StreamParsor::Parse(StreamInfo& info, std::string& err)
{
    if (_url.empty())
    {
        err = "empty stream url";
    } 
    else
    {
        std::map<std::string, std::string> avOptions;
        avOptions.insert(std::make_pair("buffer_size", "1024000"));
        avOptions.insert(std::make_pair("stimeout", "20000000"));
        avOptions.insert(std::make_pair("analyzeduration", "20000000"));
        avOptions.insert(std::make_pair("probesize", "20000000"));

        AVDictionary* pOptions = NULL;
        for (auto opt : avOptions)
        {
            av_dict_set(&pOptions, opt.first.c_str(), opt.second.c_str(), 0);
        }

        std::stringstream ss;

        AVFormatContext* formatCtx = nullptr;
        do 
        {
            int avResult = avformat_open_input(&formatCtx, _url.c_str(), NULL, &pOptions);
            if (avResult < 0)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "open media(" << _url << ") failed: " << errstr << "(" << avResult << ")";
                break;
            }

            if (avResult = avformat_find_stream_info(formatCtx, NULL) < 0)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "find stream in media(" << _url << ") failed: " << errstr << "(" << avResult << ")";
                break;
            }

            int vindex = -1;
            // find the first video stream
            for (unsigned int i = 0; i < formatCtx->nb_streams; i++)
            {
                if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    vindex = i;
                    break;
                }
            }

            // if not found, then return failed
            if (vindex == -1)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "can not find video stream in media(" << _url << ")";
                break;
            }

            AVStream* vstream = formatCtx->streams[vindex];
            AVCodecContext* codecCtx = vstream->codec;
            if (!codecCtx)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "can not find codec context in media(" << _url << ")";
                break;
            }

            AVCodec* codec = avcodec_find_decoder(codecCtx->codec_id);
            if (!codec)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "can not find codec in media(" << _url << ") for codec id: " << codecCtx->codec_id;
                break;
            }

            if ((avResult = avcodec_open2(codecCtx, codec, NULL)) < 0)
            {
                char errstr[512] = { 0 };
                av_strerror(avResult, errstr, 512);
                ss << "open codec for media(" << _url << ") failed: " << errstr << "(" << avResult << ")";
            }

            info.fps = vstream->avg_frame_rate.num * vstream->avg_frame_rate.den;
            if (info.fps < 0.0000001f)
            {
                ss << "parse media(" << _url << ") fps failed: fps(0)";
            }
        } while (false);

        if (formatCtx)
        {
            avformat_close_input(&formatCtx);
            avformat_free_context(formatCtx);
            formatCtx = nullptr;
        }

        err = ss.str();
        if (err.empty())
        {
            info.interval = 1000.0f / info.fps;
            return true;
        }
    }
    return false;
}
