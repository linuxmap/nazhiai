
#include "DirectoryDecoder.h"
#include "AutoLock.h"
#include "Performance.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

DirectoryDecoder::DirectoryDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id)
    , _currentFramePostion(0LL), _files()
    , _decoder(nullptr), _read_buffer(nullptr)
    , _frame()
{
}

DirectoryDecoder::~DirectoryDecoder()
{
}

bool DirectoryDecoder::Init()
{
    AUTOLOCK(_decoderLocker);

    // check directory and file
    if (!FileSystem::Exist(_url))
    {
        _errorMessage = "directory(" + _url + ") can not be opened, reason: path not found";
        LOG(INFO) << _errorMessage;
        return false;
    }

    char ch = _url[_url.size() - 1];
    // check path pattern
    if (ch != '/' && ch != '\\')
    {
        _url += "/";
    }

    // use GPU
    if (_decoderParam.device_index >= 0)
    {
        int res = jpeg_codec::create_decoder(&_decoder, _decoderParam.device_index);
        if (res)
        {
            _errorMessage = "directory(" + _url + ") can not be opened, reason create jpeg decode failed: " + std::to_string(res);
            LOG(INFO) << _errorMessage;
            return false;
        }

        _read_buffer = (unsigned char*)malloc(8 << 20); // 8Mb
    }

    FileSystem::ListFile(_url, _files);
    FileSystem::OrderByName(_files);

    _errorMessage = "";
    LOG(INFO) << Name() << "(" << _id << ") create success";

    _currentFramePostion = 0;

    return true;
}

void DirectoryDecoder::Uninit()
{
    AUTOLOCK(_decoderLocker);
    if (_files.size() > 0)
    {
        _files.clear();

        if (_decoderParam.device_index >= 0 && _decoder)
        {
            jpeg_codec::destroy_decoder(_decoder);
        }
        if (_read_buffer)
        {
            free(_read_buffer);
            _read_buffer = nullptr;
        }
        LOG(INFO) << Name() << "(" << _id << ") destroy success";
    }
    else
    {
        LOG(INFO) << Name() << "(" << _id << ") is not created yet";
    }
    _currentFramePostion = 0;
}

bool DirectoryDecoder::ReadFrame(DecodedFrame& frame)
{
    if (_currentFramePostion >= _files.size())
    {
        _currentFramePostion = 0;
    }

    FileSystem::FNode& fnode = _files[_currentFramePostion++];
    std::fstream fs(_url + fnode.info.name, std::ios::in | std::ios::binary);

    frame.position = _currentFramePostion;
#if 1
    if (!fs.is_open())
    {
        fs.clear();
        LOG(ERROR) << _url + fnode.info.name << " open failed";
        return false;
    }
    else
    {
        frame.id = _currentFramePostion;
        frame.imdata = rawptr(new std::vector<char>());
        fs.seekg(0, std::ios::end);
        auto end = fs.tellg();
        fs.seekg(0, std::ios::beg);
        frame.imdata->resize(end);
        fs.read((char *)(frame.imdata->data()), end);
        fs.close();
        return true;
    }
#else

    if (_frame.empty())
    {
        _frame = cv::imread(_url + fnode.info.name, cv::IMREAD_COLOR);
    }
    frame.mat = _frame;
    return !frame.mat.empty();
#endif
}



