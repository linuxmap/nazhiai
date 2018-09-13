
#include "DirectoryDecoder.h"

#include "Performance.h"

DirectoryDecoder::DirectoryDecoder(const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id)
    : BaseDecoder(url, decoderParam, decodeParam, id)
    , _currentFramePostion(0LL), _files()
{
}

DirectoryDecoder::~DirectoryDecoder()
{
}

void DirectoryDecoder::Create() throw(BaseException)
{
    // check directory and file
    if (!FileSystem::Exist(_url)) {
        throw BaseException(0, "directory(" + _url + ") can not be opened");
    }

    FileSystem::ListFile(_url, _files);
    FileSystem::OrderByName(_files);

    // create decode thread
    BaseDecoder::Create();
}

void DirectoryDecoder::Destroy() throw(BaseException)
{
    _files.clear();

    // destroy decode thread
    BaseDecoder::Destroy();
}

bool DirectoryDecoder::ReadFrame(cv::Mat& frame)
{
    if (_currentFramePostion >= _files.size())
    {
        _currentFramePostion = 0;
    }

    FileSystem::FNode& fnode = _files[_currentFramePostion++];
    try
    {
        frame = cv::imread(_url + fnode.info.name);
    }
    catch (cv::Exception& ex)
    {
        PRINT("%s failed: %s\n", __FUNCTION__, ex.what());
        return false;
    }
    return true;
}

bool DirectoryDecoder::ReadFrame(cv::cuda::GpuMat& frame)
{
    cv::Mat mat;
    if (ReadFrame(mat))
    {
        frame.upload(mat);
        return true;
    } 
    else
    {
        return false;
    }
}

