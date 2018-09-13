

#include "facecapture.h"
#include "SnapCamera.h"
#include "FaceCaptureContext.h"
#include "ErrorHandler.h"

#include <windows.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#include "glog/logging.h"

#ifdef NDEBUG
#pragma comment(lib, "glog.lib")
#else
#pragma comment(lib, "glogd.lib")
#endif

static volatile bool module_initilized = false;

FACECAPUTRE_C_API const char* GetLastCaptureError()
{
    return ErrorHandler::GetLastError();
}

static void FailureWriter(const char *data, int size)
{
    LOG(FATAL) << std::string(data, size);
}

#ifdef DUMP_RELEASE

#include <time.h>
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    // 返回EXCEPTION_CONTINUE_SEARCH，让程序停止运行   
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    
    HANDLE hFile = ::CreateFile(L"facecapture_crash.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION ExInfo;

        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = NULL;

        // write the dump
        BOOL bOK = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
        ret = EXCEPTION_EXECUTE_HANDLER;
        ::CloseHandle(hFile);
    }

    return ret;
}

#endif

FACECAPUTRE_C_API bool FaceCaptureInit(const char* path)
{
    static bool LOGGER_INITIALIZED = false;
    if (!LOGGER_INITIALIZED)
    {
        LOGGER_INITIALIZED = true;

#ifdef DUMP_RELEASE
        ::SetUnhandledExceptionFilter(TopLevelFilter);
#endif

        google::InitGoogleLogging("facecapture");
        google::SetStderrLogging(google::GLOG_INFO);
        FLAGS_log_dir = "logs";
        FLAGS_colorlogtostderr = true;  // Set log color  
        FLAGS_logbufsecs = 0;  // Set log output speed(s)  
        FLAGS_max_log_size = 128;  // Set max log file size  
        FLAGS_stop_logging_if_full_disk = true;  // If disk is full  
        google::InstallFailureWriter(&FailureWriter);
        google::InstallFailureSignalHandler();

        LOG(INFO) << "         ======================== version: facecapture-1.4.0.180912 =============================";
    }

    if (!module_initilized)
    {
        module_initilized = true;

        if (!DetectInit())
        {
            ErrorHandler::SetLastError(GetLastDetectError());
            return false;
        }

        if (!DecodeInit())
        {
            ErrorHandler::SetLastError(GetLastDecodeError());
            return false;
        }

        // read configuration
        if (!FaceCaptureContext::ReadConfiguration(path))
        {
            return false;
        }

        // show configuration information
        FaceCaptureContext::ShowConfiguration();

        if (!FaceCaptureContext::InitializeFaceExtractors())
        {
            FaceCaptureContext::UninitializeFaceExtractors();
            return false;
        }

        if (!FaceCaptureContext::InitializeFaceDetectors())
        {
            FaceCaptureContext::UninitializeFaceDetectors();
            return false;
        }

        return true;
    }
    else
    {
        ErrorHandler::SetLastError("facecapture can not be initialized more than once");
    }
    return false;
}

FACECAPUTRE_C_API void FaceCaptureUnInit()
{
    if (module_initilized)
    {
        module_initilized = false;

        FaceCaptureContext::UninitializeAllAsyncDecoders();
        FaceCaptureContext::UninitializeAllDecoders();
        FaceCaptureContext::UninitializeAllSnapMachines();

        FaceCaptureContext::UninitializeFaceDetectors();
        FaceCaptureContext::UninitializeFaceExtractors();

        DecodeDestroy();
        DetectDestroy();
    } 
}

FACECAPUTRE_C_API bool OpenSnapCamera(int deviceIndex, int cameraType, const char* ip, unsigned short port, const char* username, const char* password, const FaceParam& faceParam)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(deviceIndex);
    if (faceDetector)
    { 
        SnapMachine* snapMachine = CreateSnapMachine(faceDetector, cameraType, faceParam, ip, port, username, password);
        if (snapMachine)
        {
            std::string id = GetSnapMachineId(snapMachine);
            if (FaceCaptureContext::FindSnapMachine(id))
            {
                ErrorHandler::ErrorStream << "snap camera already created: " << id;
                ErrorHandler::FlushLastErrorStream();

                DestroySnapMachine(snapMachine);
                return false;
            }
            if (!StartSnapMachine(snapMachine))
            {
                ErrorHandler::ErrorStream << "snap camera start failed: " << id;
                ErrorHandler::FlushLastErrorStream();

                DestroySnapMachine(snapMachine);
                return false;
            }

            FaceCaptureContext::AddSnapMachine(id, snapMachine);
        }
        else
        {
            ErrorHandler::ErrorStream << "snap camera created failed: " << ip << ":" << port;
            ErrorHandler::FlushLastErrorStream();
            return false;
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << deviceIndex << "] for id: " << ip << ":" << port;
        ErrorHandler::FlushLastErrorStream();
    }
    return false;
}

FACECAPUTRE_C_API bool CloseSnapCamera(const char* ip, unsigned short port)
{
    std::string id = std::string(ip) + "_" + std::to_string(port);
    return FaceCaptureContext::DelSnapMachine(id.c_str());
}

FACECAPUTRE_C_API bool OpenImageCamera(int deviceIndex, const char* id, const FaceParam& faceParam)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(deviceIndex);
    if (faceDetector)
    {
        if (FaceCaptureContext::FindSnapMachine(id))
        {
            ErrorHandler::ErrorStream << "image camera already created: " << id;
            ErrorHandler::FlushLastErrorStream();
            return false;
        }
        SnapMachine* snapMachine = CreateSnapMachine(faceDetector, faceParam, id);
        if (snapMachine)
        {
            if (!StartSnapMachine(snapMachine))
            {
                ErrorHandler::ErrorStream << "image camera start failed: " << id;
                ErrorHandler::FlushLastErrorStream();

                DestroySnapMachine(snapMachine);
                return false;
            }

            FaceCaptureContext::AddSnapMachine(id, snapMachine);
        }
        else
        {
            ErrorHandler::ErrorStream << "image camera create failed: " << id;
            ErrorHandler::FlushLastErrorStream();
            return false;
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << deviceIndex << "] for id: " << id;
        ErrorHandler::FlushLastErrorStream();
        return false;
    }
    return true;
}

FACECAPUTRE_C_API bool FeedImageCamera(const ImageParam& imageParam)
{
    return FaceCaptureContext::FeedImageMachine(imageParam);
}

FACECAPUTRE_C_API bool CloseImageCamera(const char* id)
{
    return FaceCaptureContext::DelSnapMachine(id);
}

FACECAPUTRE_C_API bool OpenStreamsAsync(const std::vector<int>& types, const std::vector<std::string>& urls, const std::vector<DecoderParam>& decoderParams, const std::vector<DecodeParam>& decodeParams, const std::vector<FaceParam>& faceParams, const std::vector<std::string>& ids, StreamAsyncCallback callback)
{
    if (types.size() == urls.size()
        && urls.size() == decoderParams.size() 
        && decoderParams.size() == decodeParams.size() 
        && decodeParams.size() == faceParams.size() 
        && faceParams.size() == ids.size())
    {
        if (urls.size() > 0)
        {
            FaceCaptureContext::AddAsyncDecoders(types, urls, decoderParams, decodeParams, faceParams, ids, callback);
            return true;
        }
        else
        {
            ErrorHandler::SetLastError("parameters are empty");
        }
    }
    else
    {
        ErrorHandler::SetLastError("parameters have different size");
    }
    return false;
}

FACECAPUTRE_C_API bool CloseStreamsAsync(const std::vector<std::string>& ids, StreamAsyncCallback callback)
{
    FaceCaptureContext::DelAsyncDecoders(ids, callback);
    return true;
}

FACECAPUTRE_C_API bool OpenRTSP(const char* url, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(decoderParam.device_index);
    if (faceDetector)
    {
        if (!FaceCaptureContext::FindDecoder(id))
        {
            BaseDecoder* baseDecdoer = OpenRTSP(url, decoderParam, decodeParam, id, false, FaceCaptureContext::StreamStoppedCallback);
            if (baseDecdoer)
            {
                AddSource(faceDetector, baseDecdoer, faceParam);
                FaceCaptureContext::AddDecoder(id, baseDecdoer);
                return true;
            }
            else
            {
                ErrorHandler::SetLastError(GetLastDecodeError());
            }
        }
        else
        {
            ErrorHandler::ErrorStream << "RTSP already created: " << id;
            ErrorHandler::FlushLastErrorStream();
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << decoderParam.device_index << "] for id: " << id;
        ErrorHandler::FlushLastErrorStream();
    }
    return false;
}

FACECAPUTRE_C_API bool OpenVideo(const char* path, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(decoderParam.device_index);
    if (faceDetector)
    {
        if (!FaceCaptureContext::FindDecoder(id))
        {
            BaseDecoder* baseDecdoer = OpenVideo(path, decoderParam, decodeParam, id, false, FaceCaptureContext::StreamStoppedCallback);
            if (baseDecdoer)
            {
                AddSource(faceDetector, baseDecdoer, faceParam);
                FaceCaptureContext::AddDecoder(id, baseDecdoer);
                return true;
            }
            else
            {
                ErrorHandler::SetLastError(GetLastDecodeError());
            }
        }
        else
        {
            ErrorHandler::ErrorStream << "video already created: " << id;
            ErrorHandler::FlushLastErrorStream();
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << decoderParam.device_index << "] for id: " << id;
        ErrorHandler::FlushLastErrorStream();
    }
    return false;
}

FACECAPUTRE_C_API bool OpenUSB(const char* path, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(decoderParam.device_index);
    if (faceDetector)
    {
        if (!FaceCaptureContext::FindDecoder(id))
        {
            BaseDecoder* baseDecdoer = OpenUSB(path, decoderParam, decodeParam, id, FaceCaptureContext::StreamStoppedCallback);
            if (baseDecdoer)
            {
                AddSource(faceDetector, baseDecdoer, faceParam);
                FaceCaptureContext::AddDecoder(id, baseDecdoer);
                return true;
            }
            else
            {
                ErrorHandler::SetLastError(GetLastDecodeError());
            }
        }
        else
        {
            ErrorHandler::ErrorStream << "USB port already created: " << id;
            ErrorHandler::FlushLastErrorStream();
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << decoderParam.device_index << "] for id: " << id;
        ErrorHandler::FlushLastErrorStream();
    }
    return false;
}

FACECAPUTRE_C_API bool OpenDirectory(const char* path, const DecoderParam& decoderParam, const DecodeParam& decodeParam, const FaceParam& faceParam, const char* id)
{
    FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(decoderParam.device_index);
    if (faceDetector)
    {
        if (!FaceCaptureContext::FindDecoder(id))
        {
            BaseDecoder* baseDecdoer = OpenDirectory(path, decoderParam, decodeParam, id, FaceCaptureContext::StreamStoppedCallback);
            if (baseDecdoer)
            {
                AddSource(faceDetector, baseDecdoer, faceParam);
                FaceCaptureContext::AddDecoder(id, baseDecdoer);
                return true;
            }
            else
            {
                ErrorHandler::SetLastError(GetLastDecodeError());
            }
        }
        else
        {
            ErrorHandler::ErrorStream << "Directory already created: " << id;
            ErrorHandler::FlushLastErrorStream();
        }
    }
    else
    {
        ErrorHandler::ErrorStream << "can not find face detector instance which work on device[" << decoderParam.device_index << "] for id: " << id;
        ErrorHandler::FlushLastErrorStream();
    }
    return false;
}

FACECAPUTRE_C_API bool CloseStream(const char* id)
{
    BaseDecoder* decoder = FaceCaptureContext::FindDecoder(id);
    if (decoder)
    {
        int deviceIndex = GetDecoderDeviceIndex(decoder);
        FaceDetector* faceDetector = FaceCaptureContext::ChooseFaceDetector(deviceIndex);
        if (faceDetector)
        {
            // delete from face detector
            DelSource(faceDetector, decoder);
        }
        else
        {
            LOG(WARNING) << "can not find face detector instance which work on device[" << deviceIndex << "] for id: " << id;
        }

        return FaceCaptureContext::DelDecoder(id);
    }
    return false;
}

FACECAPUTRE_C_API bool GetFaceCapture(std::vector<CaptureResultPtr>& captureResults)
{
    return FaceCaptureContext::GetCaptureResults(captureResults);
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

