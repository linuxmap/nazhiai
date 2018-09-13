#include "AsyncContext.h"

#include "AutoLock.h"

#include "StreamDecoder.h"
#include "FaceDetector.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

const std::string AsyncContext::SUCCESS = "success";
const std::string AsyncContext::FAILED = "failed";

static const char STREAM_NAME[][32] = { "network_stream", "local_file_stream", "usb_camera_stream", "direcotry_file_stream" };

std::string AsyncContext::FailedReason(const std::string& des)
{
    return FAILED + ":" + des;
}

AsyncContext::AsyncContext(int gpuIndex, FaceDetector* faceDetector)
    : _faceDetector(faceDetector), _gpuIndex(gpuIndex), _retrieveThread(), _retrieving(false)
    , _baseDecoders()
    , _operationsLocker(), _operations()
{
}

AsyncContext::~AsyncContext()
{

}

void AsyncContext::Create()
{
    AUTOLOCK(_operationsLocker);
    _retrieving = true;
    _retrieveThread = std::thread(&AsyncContext::RetrieveFrame, this);
}

void AsyncContext::Destroy()
{
    AUTOLOCK(_operationsLocker);
    if (_retrieving)
    {
        _retrieving = false;
        WAIT_TO_EXIT(_retrieveThread);
    }
}

bool AsyncContext::AddAsyncDecoder(int streamType, const std::string& url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const std::string& id, const FaceParam& faceParam, void(*asyncCallback)(const std::string&, const std::string&), void(*stoppedCallback)(const std::string&, bool))
{
    OperInfo info;
    info.operationType = OPER_ADD;
    info.streamType = streamType;
    info.url = url;
    info.decoderParam = decoderParam;
    info.decodeParam = decodeParam;
    info.id = id;
    info.faceParam = faceParam;
    info.asyncCallback = asyncCallback;
    info.stoppedCallback = stoppedCallback;

    AUTOLOCK(_operationsLocker);
    _operations.push(info);
    return true;
}

bool AsyncContext::DelAsyncDecoder(const std::string& id, void(*asyncCallback)(const std::string&, const std::string&))
{
    OperInfo info;
    info.operationType = OPER_DEL;
    info.id = id;
    info.asyncCallback = asyncCallback;

    AUTOLOCK(_operationsLocker);
    _operations.push(info);
    return true;
}

bool AsyncContext::FindAsyncDecoder(const std::string& id)
{
    for (size_t idxFrame = 0; idxFrame < _baseDecoders.size(); ++idxFrame)
    {
        if (GetDecoderId(_baseDecoders[idxFrame]) == id)
        {
            return true;
        }
    }
    return false;
}

void AsyncContext::RetrieveFrame()
{
    LOG(INFO) << "async context create success";
    while (_retrieving)
    {
        OperInfo info;
        do // operation
        {
            AUTOLOCK(_operationsLocker);
            if (_operations.size())
            {
                info = _operations.front();
                _operations.pop();
            }
        } while (false);

        // check if any operation 
        if (info.operationType != OPER_NONE)
        {
            // addition
            if (info.operationType == OPER_ADD)
            {
                CreateAsyncDecoder(info);
            }
            else
            {
                DestroyAsyncDecoder(info);
            }
        }

        int retrievedFrameNumber = 0;
        for (size_t idxFrame = 0; idxFrame < _baseDecoders.size(); ++idxFrame)
        {
            if (DecodeFrame(_baseDecoders[idxFrame]))
            {
                ++retrievedFrameNumber;
            }
        }

        if (retrievedFrameNumber <= 0)
        {
            // DecodeFrame(_baseDecoders[100]); // testing
            // if no frame retrieved
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    for each(BaseDecoder* baseDecoderPtr in _baseDecoders)
    {
        DelSource(_faceDetector, baseDecoderPtr);
        CloseDecoder(baseDecoderPtr);
    }
    _baseDecoders.clear();

    LOG(INFO) << "async context destroy success";
}

void AsyncContext::CreateAsyncDecoder(OperInfo& info)
{
    std::string result;
    bool found = false;
    for (size_t idxDecoders = 0; idxDecoders < _baseDecoders.size(); ++idxDecoders)
    {
        if (GetDecoderId(_baseDecoders[idxDecoders]) == info.id)
        {
            result = FailedReason("decoder has already existed");
            found = true;
            break;
        }
    }

    // not exists
    if (!found)
    {
        BaseDecoder* baseDecoder = nullptr;
        if (NETWORK_STREAM == info.streamType)
        {
            baseDecoder = OpenRTSP(info.url.c_str(), info.decoderParam, info.decodeParam, info.id.c_str(), true, info.stoppedCallback);
        }
        else
        {
            result = FailedReason(std::string("unsupported media type: ") + STREAM_NAME[info.streamType]);
            LOG(ERROR) << "unsupported async media type: " << STREAM_NAME[info.streamType];
        }

        if (baseDecoder)
        {
            AddSource(_faceDetector, baseDecoder, info.faceParam);
            _baseDecoders.push_back(baseDecoder);

            result = SUCCESS;
        }
        else
        {
            std::string err = GetLastDecodeError();
            result = FailedReason(err);
            LOG(ERROR) << err;
        }
    }

    if (info.asyncCallback)
    {
        info.asyncCallback(info.id, result);
    }
}

void AsyncContext::DestroyAsyncDecoder(OperInfo& info)
{
    std::string result;
    BaseDecoder* baseDecoder(nullptr);
    for (size_t idxDecoders = 0; idxDecoders < _baseDecoders.size(); ++idxDecoders)
    {
        if (GetDecoderId(_baseDecoders[idxDecoders]) == info.id)
        {
            baseDecoder = _baseDecoders[idxDecoders];
            _baseDecoders.erase(_baseDecoders.begin() + idxDecoders);
            break;
        }
    }

    // found
    if (baseDecoder)
    {
        DelSource(_faceDetector, baseDecoder);
        CloseDecoder(baseDecoder);

        result = SUCCESS;
    }
    else
    {
        result = FailedReason("stream does not exist");
    }

    if (info.asyncCallback)
    {
        info.asyncCallback(info.id, result);
    }
}




